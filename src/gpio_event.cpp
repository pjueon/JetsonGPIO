
#include "private/gpio_event.h"

#include <fcntl.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <time.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <deque>
#include <map>
#include <mutex>
#include <thread>
#include <vector>

#include "JetsonGPIO.h"

#define MAX_EPOLL_EVENTS 20
#define READ_SIZE 10

namespace GPIO {

  std::map<EventResultCode, const char *> event_error_msg = {
      {EventResultCode::SysFD_EdgeOpen, "Failure to open the /sys/class/gpio/gpio{$ch}/edge file"},
      {EventResultCode::UnallowedEdgeNone, "Specifying Edge as 'none' was not allowed"},
      {EventResultCode::IllegalEdgeArgument, "Illegal Edge argument"},
      {EventResultCode::SysFD_EdgeWrite, "Failure to write to the /sys/class/gpio/gpio{$ch}/edge file"},
      {EventResultCode::SysFD_ValueOpen, "Failure to open the channels System value file descriptor"},
      {EventResultCode::SysFD_ValueNonBlocking,
       "Failure to set to non-blocking the channels System value file descriptor"},
      {EventResultCode::ChannelAlreadyBlocked,
       "This channel is already being blocked (Probably by a concurrent wait_for_edge call)"},
      {EventResultCode::ConflictingEdgeType, "Already opened channel is currently detecting a different edge type"},
      {EventResultCode::ConflictingBounceTime, "Already opened channel is currently employing a different bounce time"},
      {EventResultCode::InternalTrackingError, "Internal Event Tracking Error"},
      {EventResultCode::EpollFD_CreateError, "Failed to create the EPOLL file descriptor"},
      {EventResultCode::EpollCTL_Add, "Failure to add an event to the EPOLL file descriptor"},
      {EventResultCode::EpollWait, "Error occurred during call to epoll_wait"},
      {EventResultCode::GPIO_Event_Not_Found,
       "A channel event was not added to add a callback to. Call add_event_detect() first"},
  };

  void remove_edge_detect(int gpio);

  typedef struct __gpioEventObject {
    enum ModifyEvent { NONE, ADD, REMOVE, MODIFY } _epoll_change_flag;
    struct epoll_event _epoll_event;

    int channel_id;
    int gpio;
    int fd;
    Edge edge;
    uint64_t bounce_time;
    uint64_t last_event;

    bool event_occurred;

    bool blocking;
    std::vector<void (*)(int)> callbacks;
  } _gpioEventObject;

  std::recursive_mutex _epmutex;
  std::thread *_epoll_fd_thread = nullptr;
  std::atomic_bool _epoll_run_loop;

  std::map<int, std::shared_ptr<_gpioEventObject>> _gpio_events;
  std::atomic_int _auth_event_channel_count(0);
  std::map<int, int> _fd_to_gpio_map;

  //----------------------------------

  int _write_sysfs_edge(int gpio, Edge edge, bool allow_none = true)
  {
    int result;
    char buf[256];
    snprintf(buf, 256, "%s/gpio%i/edge", _SYSFS_ROOT, gpio);
    int edge_fd = open(buf, O_WRONLY);
    if (edge_fd == -1) {
      // I/O Error
      // fprintf(stderr, "Error opening file '%s'\n", buf);
      return (int)GPIO::EventResultCode::SysFD_EdgeOpen;
    }
    switch (edge) {
      case Edge::RISING:
        result = write(edge_fd, "rising", 7);
        break;
      case Edge::FALLING:
        result = write(edge_fd, "falling", 7);
        break;
      case Edge::BOTH:
        result = write(edge_fd, "both", 7);
        break;
      case Edge::NONE: {
        if (!allow_none) {
          close(edge_fd);
          return (int)GPIO::EventResultCode::UnallowedEdgeNone;
        }
        result = write(edge_fd, "none", 7);
        break;
      }
      case Edge::UNKNOWN:
      default:
        fprintf(stderr, "Bad argument, edge=%i\n", (int)edge);
        close(edge_fd);
        return (int)GPIO::EventResultCode::IllegalEdgeArgument;
    }

    if (result == -1) {
      perror("sysfs/edge write");
      return (int)GPIO::EventResultCode::SysFD_EdgeWrite;
    }
    else {
      result = 0;
    }

    close(edge_fd);
    return result;
  }

  int _open_sysfd_value(int gpio, int &fd)
  {
    char buf[256];
    snprintf(buf, 256, "%s/gpio%i/value", _SYSFS_ROOT, gpio);
    fd = open(buf, O_RDONLY);

    if (fd == -1) {
      // printf("[DEBUG] '%s' for given gpio '%i'\n", buf, gpio);
      return (int)GPIO::EventResultCode::SysFD_ValueOpen;
    }

    // Set the file descriptor to a non-blocking usage
    int result = fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
    if (result == -1) {
      perror("fcntl");
      close(fd);
      return (int)GPIO::EventResultCode::SysFD_ValueNonBlocking;
    }

    return 0;
  }

  std::map<int, std::shared_ptr<_gpioEventObject>>::iterator
  _epoll_thread_remove_event(int epoll_fd, std::map<int, std::shared_ptr<_gpioEventObject>>::iterator geo_it)
  {
    auto geo = geo_it->second;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, geo->fd, 0) == -1) {
      fprintf(stderr, "Failed to delete file descriptor to epoll\n");
    }

    auto fg_it = _fd_to_gpio_map.find(geo->fd);
    if (fg_it != _fd_to_gpio_map.end())
      _fd_to_gpio_map.erase(fg_it);

    // Close the fd
    if (close(geo->fd) == -1) {
      fprintf(stderr, "Failed to close file descriptor\n");
    }

    // printf("[DEBUG] fd(%i:%i) removed from epoll\n", geo->channel_id, geo->gpio);

    // Erase from the map collection
    return _gpio_events.erase(geo_it);
  }

  void _epoll_thread_loop()
  {
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
      fprintf(stderr, "Failed to create epoll file descriptor\n");
      return;
    }

    int e, event_count, result;
    epoll_event events[MAX_EPOLL_EVENTS];
    while (_epoll_run_loop) {
      // Wait a small time for events
      event_count = epoll_wait(epoll_fd, events, MAX_EPOLL_EVENTS, 1);
      std::lock_guard<std::recursive_mutex> mutex_lock(_epmutex);

      // Handle Events
      if (event_count) {
        if (event_count < 0) {
          fprintf(stderr, "epoll_wait error\n");
          // TODO
          break;
        }

        // printf("[DEBUG] event_count=%i\n", event_count);

        // Handle event modifications
        auto tick = (uint64_t)std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch())
                        .count();

        // Iterate through each collected event
        for (e = 0; e < event_count; e++) {
          // Obtain the event object for the event
          auto gpio_it = _fd_to_gpio_map.find(events[e].data.fd);
          if (gpio_it == _fd_to_gpio_map.end()) {
            // puts("[DEBUG] Couldn't find gpio for fd-event");
            // Ignore it
            continue;
          }

          auto geo_it = _gpio_events.find(gpio_it->second);
          if (geo_it == _gpio_events.end()) {
            // TODO -- SHOULDN"T HAPPEN
            printf("[DEBUG] Couldn't find gpio object with gpio %i (OK if gpio edge was removed)\n", gpio_it->second);
            continue;
          }

          if (geo_it->second->_epoll_change_flag != _gpioEventObject::ModifyEvent::NONE) {
            // To be dealt with later. No events should be fired
            puts("[DEBUG] _epoll_change_flag was not NONE (OK if there was a ModifyEvent)");
            continue;
          }

          auto geo = geo_it->second;

          // Check event conditions
          if (geo->bounce_time) {
            if (tick - geo->last_event < geo->bounce_time) {
              printf("[DEBUG] tick(%lu) - geo->last_event(%lu), < geo->bounce_time(%lu)\n", tick, geo->last_event,
                     geo->bounce_time);
              continue;
            }

            geo->last_event = geo->bounce_time;
          }

          // Fire event
          geo->event_occurred = true;
          for (auto cb : geo->callbacks) {
            cb(geo->channel_id);
          }
        }
      }

      // Handle changes/modifications to GPIO event objects
      for (auto geo_it = _gpio_events.begin(); geo_it != _gpio_events.end();) {
        auto geo = geo_it->second;
        switch (geo->_epoll_change_flag) {
          case _gpioEventObject::ModifyEvent::NONE: {
            // No change

            // Iterate to next element
            geo_it++;
          } break;
          case _gpioEventObject::ModifyEvent::MODIFY: {
            // For now this just means modification of the edge type, which is done on the calling thread
            // Just set back to NONE and continue
            geo->_epoll_change_flag = _gpioEventObject::ModifyEvent::NONE;

            // Iterate to next element
            geo_it++;
          } break;
          case _gpioEventObject::ModifyEvent::ADD: {
            geo->_epoll_event.events = EPOLLIN | EPOLLPRI | EPOLLET;
            geo->_epoll_event.data.fd = geo->fd;

            if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, geo->fd, &geo->_epoll_event) == -1) {
              fprintf(stderr, "Failed to add file descriptor to epoll (gpio=%i)\n", geo->gpio);
              // TODO
            }

            geo->_epoll_change_flag = _gpioEventObject::ModifyEvent::NONE;
            puts("[DEBUG] fd added to epoll");

            // Iterate to next element
            geo_it++;
          } break;
          case _gpioEventObject::ModifyEvent::REMOVE: {
            printf("Ack remove command from %i:%i\n", geo_it->first, geo_it->second->channel_id);
            geo_it = _epoll_thread_remove_event(epoll_fd, geo_it);

            // Break and do not iterate past the returned element
          } break;
          default: {
            // Iterate to next element
            geo_it++;
          } break;
        }
      }
    }

    puts("[DEBUG] EPOLL thread closing: GPIO-cleanup");
    // Thread is coming to an end
    // -- Cleanup
    // GPIO Event Objects
    {
      std::lock_guard<std::recursive_mutex> mutex_lock(_epmutex);
      for (auto geo_it = _gpio_events.begin(); geo_it != _gpio_events.end();) {
        geo_it = _epoll_thread_remove_event(epoll_fd, geo_it);
      }
    }

    puts("[DEBUG] EPOLL thread closing: epoll_fd");

    // epoll
    if (close(epoll_fd) == -1) {
      fprintf(stderr, "Failed to close epoll file descriptor\n");
    }

    // DEBUG
    puts("[DEBUG] EPOLL thread exited!");
    // DEBUG
  }

  void _epoll_start_thread()
  {
    _epoll_run_loop = true;
    _epoll_fd_thread = new std::thread(_epoll_thread_loop);
  }

  void _event_cleanup(int gpio) { remove_edge_detect(gpio); }

  //----------------------------------
  /* TODO error codes
   */
  int blocking_wait_for_edge(int gpio, int channel_id, Edge edge, uint64_t bounce_time, uint64_t timeout)
  {
    struct timespec timeout_time, current_time;
    if (timeout) {
      clock_gettime(CLOCK_REALTIME, &timeout_time);
      // printf("[DEBUG] current_time=%li;%li", timeout_time.tv_sec, timeout_time.tv_nsec);

      timeout_time.tv_nsec += (timeout % 1000) * 1e6;
      long overlap = timeout_time.tv_nsec / 1e9;
      timeout_time.tv_nsec -= overlap;

      timeout_time.tv_sec += timeout / 1000 + overlap;
      // printf("  timeout_time==%li;%li\n", timeout_time.tv_sec, timeout_time.tv_nsec);
    }

    int result;
    std::shared_ptr<_gpioEventObject> geo;
    bool remove_geo = false;
    uint64_t gpio_last_event = 0;
    {
      // Enter Mutex
      std::lock_guard<std::recursive_mutex> mutex_lock(_epmutex);

      // Ensure a conflict does not exist with the concurrent event detecting thread and its collection of gpio events
      auto find_result = _gpio_events.find(gpio);
      if (find_result != _gpio_events.end()) {
        geo = find_result->second;

        if (geo->blocking) {
          // Channel already being blocked (can only be by another thread call)
          return (int)GPIO::EventResultCode::ChannelAlreadyBlocked;
        }

        switch (geo->_epoll_change_flag) {
          case _gpioEventObject::ModifyEvent::NONE:
          case _gpioEventObject::ModifyEvent::ADD:
          case _gpioEventObject::ModifyEvent::MODIFY: {
            if (geo->edge != edge) {
              return (int)GPIO::EventResultCode::ConflictingEdgeType;
            }
            if (bounce_time && geo->bounce_time != bounce_time) {
              return (int)GPIO::EventResultCode::ConflictingBounceTime;
            }
          } break;
          case _gpioEventObject::ModifyEvent::REMOVE: {
            // The epoll thread is inbetween concurrent transactions. Modify the existing
            // object instead of removing it

            if (geo->edge != edge) {
              // Update
              geo->_epoll_change_flag = _gpioEventObject::ModifyEvent::MODIFY;
              geo->edge = edge;

              // Set Event
              result = _write_sysfs_edge(gpio, edge);
              if (result) {
                return result;
              }
            }

            // Reset GPIO
            geo->event_occurred = false;
            geo->bounce_time = 0;
            geo->last_event = 0;

            remove_geo = true;
            ++_auth_event_channel_count;
          } break;
          default:
            // Shouldn't happen
            return (int)GPIO::EventResultCode::InternalTrackingError;
        }
      }
      else {
        // Create a gpio-event-object to avoid concurrent conflicts for the channel while blocking
        geo = std::make_shared<_gpioEventObject>();
        geo->_epoll_change_flag = _gpioEventObject::ModifyEvent::ADD;
        geo->gpio = gpio;
        geo->channel_id = channel_id;
        geo->edge = edge;
        geo->bounce_time = 0;
        geo->last_event = 0;

        // Open the value fd
        result = _open_sysfd_value(gpio, geo->fd);
        if (result) {
          return result;
        }

        // Set Event
        result = _write_sysfs_edge(gpio, edge);
        if (result) {
          close(geo->fd);
          return result;
        }

        // Set
        _fd_to_gpio_map[geo->fd] = gpio;
        _gpio_events[gpio] = geo;

        remove_geo = true;
        ++_auth_event_channel_count;
      }

      geo->blocking = true;
    }

    // Execute the epoll awaiting the event
    int epoll_fd = 0, error = 0;
    result = (int)EventResultCode::None; // 0
    {
      epoll_fd = epoll_create1(0);
      if (epoll_fd == -1) {
        error = (int)GPIO::EventResultCode::EpollFD_CreateError;
        perror("epoll_create1(0):");
        goto cleanup;
      }

      geo->_epoll_event.events = EPOLLIN | EPOLLPRI | EPOLLET;
      geo->_epoll_event.data.fd = geo->fd;

      if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, geo->fd, &geo->_epoll_event) == -1) {
        close(epoll_fd);
        perror("epoll_ctl():");
        error = (int)GPIO::EventResultCode::EpollCTL_Add;
        goto cleanup;
      }

      int e, event_count;
      epoll_event events[1];
      bool initial_edge = true;
      while (1) {
        if (timeout) {
          clock_gettime(CLOCK_REALTIME, &current_time);
          if (current_time.tv_sec > timeout_time.tv_sec ||
              (current_time.tv_sec == timeout_time.tv_sec && current_time.tv_nsec >= timeout_time.tv_nsec)) {
            // Time-out
            // printf("[DEBUG] BREAK current_time=%li;%li", current_time.tv_sec, current_time.tv_nsec);
            // printf("  timeout_time==%li;%li\n", timeout_time.tv_sec, timeout_time.tv_nsec);
            break;
          }
        }

        // Wait a small time for events
        event_count = epoll_wait(epoll_fd, events, 1, 1);
        // printf("event_count=%i\n", event_count);

        // Handle Events
        if (event_count) {
          if (event_count == -1) {
            perror("epoll_wait");
            error = (int)EventResultCode::EpollWait;
            goto cleanup;
          }

          // First trigger is with current state so ignore
          if (initial_edge) {
            initial_edge = false;
            continue;
          }

          // printf("events:%u\n", events[0].events);

          if (events[0].data.fd == geo->fd) {
            auto tick = (uint64_t)std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::system_clock::now().time_since_epoch())
                            .count();

            if (!geo->bounce_time || tick - geo->last_event > geo->bounce_time) {
              // TODO -- debug this ? with another concurrent event to make sure that fires too?
              //  gpio_obj.lastcall = time
              result = (int)EventResultCode::EdgeDetected;
              puts("exit B");
              break;
            }
          }
        }
      }
    }

  cleanup:
    if (epoll_fd >= 0)
      close(epoll_fd);
    geo->blocking = false;
    if (remove_geo) {
      remove_edge_detect(gpio);
    }

    if (error)
      return error;
    // printf("[DEBUG] blocking_wait_for_edge() = %i %s\n", result, (result == 0) ? "timeout" : "event");
    return result;
  }

  bool edge_event_detected(int gpio)
  {
    bool result = false;

    // Enter Mutex
    std::lock_guard<std::recursive_mutex> mutex_lock(_epmutex);

    std::shared_ptr<_gpioEventObject> geo;
    auto find_result = _gpio_events.find(gpio);
    if (find_result != _gpio_events.end()) {
      geo = find_result->second;
      result = geo->event_occurred;
      geo->event_occurred = false;
    }

    return result;
  }

  int add_edge_detect(int gpio, int channel_id, Edge edge, uint64_t bounce_time)
  {
    int result;

    // Enter Mutex
    std::lock_guard<std::recursive_mutex> mutex_lock(_epmutex);

    // auto geo;
    std::shared_ptr<_gpioEventObject> geo;
    auto find_result = _gpio_events.find(gpio);
    if (find_result != _gpio_events.end()) {
      geo = find_result->second;

      switch (geo->_epoll_change_flag) {
        case _gpioEventObject::ModifyEvent::NONE:
        case _gpioEventObject::ModifyEvent::ADD:
        case _gpioEventObject::ModifyEvent::MODIFY: {
          if (geo->edge != edge) {
            return (int)GPIO::EventResultCode::ConflictingEdgeType;
          }
          if (!bounce_time && geo->bounce_time != bounce_time) {
            return (int)GPIO::EventResultCode::ConflictingBounceTime;
          }

          // Otherwise, do nothing more
        } break;
        case _gpioEventObject::ModifyEvent::REMOVE: {
          // The epoll thread is inbetween concurrent transactions. Modify the existing
          // object instead of removing it

          if (geo->edge != edge) {
            // Update
            geo->_epoll_change_flag = _gpioEventObject::ModifyEvent::MODIFY;
            geo->edge = edge;

            // Set Event
            result = _write_sysfs_edge(gpio, edge);
            if (result) {
              return result;
            }
          }
          geo->bounce_time = bounce_time;
          geo->last_event = 0;
          ++_auth_event_channel_count;
        } break;
        default:
          // Shouldn't happen
          return (int)GPIO::EventResultCode::InternalTrackingError;
      }
    }
    else {
      // Configure anew
      geo = std::make_shared<_gpioEventObject>();
      geo->_epoll_change_flag = _gpioEventObject::ModifyEvent::ADD;
      geo->gpio = gpio;
      geo->channel_id = channel_id;
      geo->edge = edge;
      geo->bounce_time = bounce_time;
      geo->last_event = 0;
      geo->blocking = false;
      geo->event_occurred = false;

      // Open the value fd
      result = _open_sysfd_value(gpio, geo->fd);
      if (result) {
        return result;
      }
      _fd_to_gpio_map[geo->fd] = gpio;

      // Set Event
      result = _write_sysfs_edge(gpio, edge);
      if (result) {
        close(geo->fd);
        return result;
      }

      // Set
      _gpio_events[gpio] = geo;
      ++_auth_event_channel_count;
    }

    if (!_epoll_fd_thread) {
      _epoll_start_thread();
    }

    return 0;
  }

  void remove_edge_detect(int gpio)
  {
    // printf("[DEBUG]remove_edge_detect(%i)\n", gpio);

    // Enter Mutex
    std::unique_lock<std::recursive_mutex> mutex_lock(_epmutex);

    auto find_result = _gpio_events.find(gpio);
    if (find_result != _gpio_events.end()) {
      auto geo = find_result->second;
      geo->_epoll_change_flag = _gpioEventObject::ModifyEvent::REMOVE;

      --_auth_event_channel_count;
      if (_auth_event_channel_count == 0 && _epoll_fd_thread) {
        // // DEBUG
        // printf("[DEBUG]_epoll_fd_thread=%p shutting down\n", _epoll_fd_thread);
        // // DEBUG

        // Signal shutdown of thread
        _epoll_run_loop = false;
        mutex_lock.unlock();

        // Wait to join
        _epoll_fd_thread->join();

        // Resume lock and clear thread
        mutex_lock.lock();
        delete _epoll_fd_thread;
        _epoll_fd_thread = nullptr;
      }

      // // DEBUG
      // printf("[DEBUG]remove_edge_detect(%i) REMOVED-ITEM\n", gpio);
      // // DEBUG
    }
    // // DEBUG
    // else
    //   printf("[DEBUG]remove_edge_detect(%i) REDUNDANT-CALL\n", gpio);
    // // DEBUG
  }

  int add_edge_callback(int gpio, void (*callback)(int))
  {
    std::lock_guard<std::recursive_mutex> mutex_lock(_epmutex);

    auto find_result = _gpio_events.find(gpio);
    if (find_result == _gpio_events.end()) {
      return (int)GPIO::EventResultCode::GPIO_Event_Not_Found;
    }

    auto geo = find_result->second;
    geo->callbacks.push_back(callback);
  }

  void remove_edge_callback(int gpio, void (*callback)(int))
  {
    std::lock_guard<std::recursive_mutex> mutex_lock(_epmutex);

    auto find_result = _gpio_events.find(gpio);
    if (find_result == _gpio_events.end()) {
      // Couldn't find it, Doesn't matter.
      return;
    }

    auto geo = find_result->second;
    for (auto geo_it = geo->callbacks.begin(); geo_it != geo->callbacks.end();) {
      if (*geo_it == callback) {
        geo->callbacks.erase(geo_it);
      }
      else {
        geo_it++;
      }
    }
  }

} // namespace GPIO