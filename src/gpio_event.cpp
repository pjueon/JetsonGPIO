
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
typedef struct __gpioEventObject {
  enum ModifyEvent { NONE, ADD, REMOVE, MODIFY } _epoll_change_flag;
  struct epoll_event _epoll_event;

  int channel;
  int fd;
  Edge edge;
  uint64_t bounce_time;
  uint64_t previous_event;

  std::vector<void (*)(int, Edge)> callbacks;
} _gpioEventObject;

// typedef struct __gpioEventQueueRequest {
//   enum RequestType {
//     Add,
//     AddCallback,
//     Remove,
//     RemoveCallback,
//   };

//   int channel;
//   RequestType request;

//   union {
//     struct {
//       Edge edge;
//       uint64_t bounce_time;
//     };
//     void (*callback)(int, Edge);
//   };
// } _gpioEventQueueRequest;

std::mutex _epmutex;
std::thread *_epoll_fd_thread = nullptr;
std::atomic_bool _epoll_run_loop;

std::map<int, std::shared_ptr<_gpioEventObject>> _gpio_events;
std::map<int, int> _fd_to_gpio_map;

// std::deque<_gpioEventQueueRequest> _gpio_event_queue;

//----------------------------------

int _write_sysfs_edge(int channel, Edge edge, bool allow_none = true)
{
  char buf[256];
  snprintf(buf, 256, "%s/gpio%i/edge", _SYSFS_ROOT, channel);
  int edge_fd = open(buf, O_WRONLY);
  if (edge_fd == -1) {
    // Error opening up edge file
    return -3;
  }
  switch (edge) {
    case Edge::RISING:
      write(edge_fd, "rising", 7);
      break;
    case Edge::FALLING:
      write(edge_fd, "falling", 7);
      break;
    case Edge::BOTH:
      write(edge_fd, "both", 7);
      break;
    case Edge::NONE: {
      if (!allow_none) {
        close(edge_fd);
        return -4;
      }
      write(edge_fd, "none", 7);
      break;
    }
    case Edge::UNKNOWN:
    default:
      close(edge_fd);
      return -5;
  }

  close(edge_fd);
  return 0;
}

int _open_sysfd_value(int channel)
{
  char buf[256];
  snprintf(buf, 256, "%s/gpio%i/value", _SYSFS_ROOT, channel);
  return open(buf, O_RDONLY);
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
  while (true) {
    // Wait a small time for events
    event_count = epoll_wait(epoll_fd, events, MAX_EPOLL_EVENTS, 1);
    if (event_count) {
      if (event_count < 0) {
        fprintf(stderr, "epoll_wait error\n");
        // TODO
        break;
      }

      // Handle event modifications
      _epmutex.lock();
      auto tick = (uint64_t)std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::system_clock::now().time_since_epoch())
                      .count();

      // Iterate through each collected event
      for (e = 0; e < event_count; e++) {
        // Obtain the event object for the event
        auto gpio_it = _fd_to_gpio_map.find(events[e].data.fd);
        if (gpio_it == _fd_to_gpio_map.end()) {
          continue;
        }

        auto geo_it = _gpio_events.find(gpio_it->second);
        if (geo_it == _gpio_events.end()) {
          // TODO -- SHOULDN"T HAPPEN
        }

        if (geo_it->second->_epoll_change_flag != _gpioEventObject::NONE) {
          // To be dealt with later. No events should be fired
          continue;
        }

        auto geo = geo_it->second;

        // Check event conditions
        if (geo->bounce_time) {
          if (tick - geo->previous_event < geo->bounce_time)
            continue;

          geo->previous_event = geo->bounce_time;
        }

        // Fire event callback(s)
        for (auto cb : geo->callbacks) {
          cb(geo->channel, geo->edge);
        }
      }
    }

    // Handle changes/modifications to GPIO event objects
    for (auto geo_it = _gpio_events.begin(); geo_it != _gpio_events.end();) {
      auto geo = geo_it->second;
      switch (geo->_epoll_change_flag) {
        case _gpioEventObject::NONE:
        case _gpioEventObject::MODIFY:
          // No change
          break;
        case _gpioEventObject::ADD: {
          geo->_epoll_event.events = EPOLLIN | EPOLLPRI | EPOLLET;
          geo->_epoll_event.data.fd = geo->fd;

          if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, geo->fd, &geo->_epoll_event) == -1) {
            fprintf(stderr, "Failed to add file descriptor to epoll\n");
            // TODO
          }
        } break;
        case _gpioEventObject::REMOVE: {
          if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, geo->fd, 0) == -1) {
            fprintf(stderr, "Failed to delete file descriptor to epoll\n");
          }

          // Close the fd
          if (close(geo->fd) == -1) {
            fprintf(stderr, "Failed to close file descriptor\n");
          }

          // Erase from the map collection
          geo_it = _gpio_events.erase(geo_it);

          // Continue and do not iterate past the returned element
          continue;
        }
      }

      // Iterate to next element
      geo_it++;
    }
    _epmutex.unlock();
  }

  // Thread is coming to an end
  // -- Cleanup
  // GPIO Event Objects
  for (auto geo_it = _gpio_events.begin(); geo_it != _gpio_events.end();) {
    auto geo = geo_it->second;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, geo->fd, 0) == -1) {
      fprintf(stderr, "Failed to delete file descriptor to epoll\n");
    }

    // Close the fd
    if (close(geo->fd) == -1) {
      fprintf(stderr, "Failed to close file descriptor\n");
    }

    // Erase from the map collection
    geo_it = _gpio_events.erase(geo_it);
  }

  // epoll
  if (close(epoll_fd) == -1) {
    fprintf(stderr, "Failed to close epoll file descriptor\n");
  }

  //   printf("%d ready events\n", event_count);
  //   for (i = 0; i < event_count; i++) {
  //     if (events[i].data.fd == gfd) {
  //       // printf("here\n");
  //       printf("Reading file descriptor '%d' (%u) -- \n", events[i].data.fd, events[i].data.u32);
  //       bytes_read = read(events[i].data.fd, read_buffer, READ_SIZE);
  //       if (bytes_read == -1) {
  //         if (EAGAIN == errno || EWOULDBLOCK == errno) {
  //           printf("EAGAIN\n");
  //           continue;
  //         }

  //         if (EINTR == errno) {
  //           printf("EINTR\n");
  //           break;
  //         }

  //         perror("read() failed");

  //         running = event_count = 0;
  //         break;
  //       }
  //       else {
  //         printf("%zd bytes read.\n", bytes_read);
  //         read_buffer[bytes_read] = '\0';
  //         printf("Read '%s'\n", read_buffer);
  //       }
  //     }
  //   }
  // }
}

void _epoll_start_thread()
{
  _epoll_run_loop = true;
  _epoll_fd_thread = new std::thread(_epoll_thread_loop);
}

int _event_cleanup(int channel)
{
  _epoll_run_loop = false;
  _epoll_fd_thread->join();

  delete _epoll_fd_thread;
  _epoll_fd_thread = nullptr;
}

//----------------------------------

void blocking_wait_for_edge();

int add_edge_detect(int channel, Edge edge, uint64_t bounce_time = 0)
{
  int result;

  // Enter Mutex
  _epmutex.lock();

  if (!_epoll_fd_thread) {
    _epoll_start_thread();
  }

  // auto geo;
  std::shared_ptr<_gpioEventObject> geo;
  auto find_result = _gpio_events.find(channel);
  if (find_result != _gpio_events.end()) {
    geo = find_result->second;

    if (geo->_epoll_change_flag == _gpioEventObject::REMOVE) {
      // The epoll thread is inbetween concurrent transactions. Modify the existing
      // object instead of removing it

      if (geo->edge != edge) {
        // Update
        geo->_epoll_change_flag = _gpioEventObject::MODIFY;
        geo->edge = edge;
        // Set Event
        result = _write_sysfs_edge(channel, edge);
        if (result) {
          _epmutex.unlock();
          return result;
        }
      }
      geo->bounce_time = bounce_time;
      geo->previous_event = 0;
    }
    else if (geo->edge != edge) {
      // Cannot have conflicting event types for a single channel
      _epmutex.unlock();
      return -1;
    }
    else if (!bounce_time && geo->bounce_time != bounce_time) {
      // Cannot have multiple conflicting bounce_times for a single channel
      _epmutex.unlock();
      return -2;
    }
  }
  else {
    // Configure anew
    geo = std::make_shared<_gpioEventObject>();
    geo->_epoll_change_flag = _gpioEventObject::ADD;
    geo->channel = channel;
    geo->edge = edge;
    geo->bounce_time = bounce_time;
    geo->previous_event = 0;

    // Open the value fd
    geo->fd = _open_sysfd_value(channel);
    if (geo->fd == -1) {
      _epmutex.unlock();
      return result;
    }

    // Set Event
    result = _write_sysfs_edge(channel, edge);
    if (result) {
      _epmutex.unlock();
      return result;
    }

    // Set
    _gpio_events[channel] = geo;
  }
  _epmutex.unlock();
}

void remove_edge_detect(int channel)
{
  // Enter Mutex
  _epmutex.lock();

  auto find_result = _gpio_events.find(channel);
  if (find_result != _gpio_events.end()) {
    auto geo = find_result->second;
    geo->_epoll_change_flag = _gpioEventObject::REMOVE;
  }

  _epmutex.unlock();
}

void add_edge_callback(int channel, void (*callback)(int, Edge));
void remove_edge_callback(int channel, void (*callback)(int, Edge));

void wait_for_edge(int channel, Edge edge, unsigned long bounce_time = 0, unsigned long timeout = 0) {}
}  // namespace GPIO