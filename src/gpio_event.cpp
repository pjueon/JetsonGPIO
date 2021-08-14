
#include <chrono>
#include <fcntl.h>
#include <map>
#include <mutex>
#include <stdio.h>
#include <sys/epoll.h>
#include <thread>
#include <time.h>
#include <unistd.h>
#include <vector>

#include "JetsonGPIO.h"

#define MAX_EPOLL_EVENTS 20
#define READ_SIZE 10

namespace GPIO {
  typedef struct _gpioEventObject {
    int channel;
    int fd;
    Edge edge;
    uint64_t bounce_time;

    std::vector<void (*)(int, Edge)> callbacks;
  } GPIOEventObject;

  typedef struct _gpio_event_queue_request {

  } GPIOEventQueueRequest;

  std::mutex _epmutex;
  std::thread *_epoll_fd_thread = nullptr;

  std::map<int, std::shared_ptr<GPIOEventObject>> _gpio_events;
  std::map<int, int> _fd_to_gpio_map;

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
        write(edge_fd, "both", 7);
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

  void blocking_wait_for_edge();

  void epoll_thread_callable()
  {
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
      fprintf(stderr, "Failed to create epoll file descriptor\n");
      return;
    }

    int e, event_count, result;
    epoll_event events[MAX_EPOLL_EVENTS];
    while (true) {

      event_count = epoll_wait(epoll_fd, events, MAX_EPOLL_EVENTS, 1);
      if (!event_count)
        continue;
      if (event_count < 0) {
        fprintf(stderr, "Failed to add file descriptor to epoll\n");
        break;
      }

      // Handle event modifications
      _epmutex.lock();
      auto tick = (uint64_t)std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::system_clock::now().time_since_epoch())
                      .count();

      printf("\niterating through events...\n");
      for (e = 0; e < event_count; e++) {

        // Obtain the event object for the event
        auto gpio_it = _fd_to_gpio_map.find(events[e].data.fd);
        if (gpio_it == _fd_to_gpio_map.end()) {
          continue;
        }

        auto geo_it = _gpio_events.find(gpio_it->second);
        if (geo_it == _gpio_events.end()) {
          // Event has been removed
          // -- Remove from epoll
          result = epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[e].data.fd, NULL);
          if (result == -1) {
            printf("TODO\n");
            perror("epoll_ctl");
            close(epoll_fd);
            return;
          }
          continue;
        }
        auto geo = geo_it->second;

        // Check event conditions
        if (geo->bounce_time) {
          if (tick - geo->previous_event < geo->bounce_time)
            continue;

          geo->previous_event = geo->bounce_time;

          TODO how is a edge modification performed on an already tracked event channel
        }

        // Fire event callback
      }
      _epmutex.unlock();

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

      // if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, gfd, 0) == -1) {
      //   fprintf(stderr, "Failed to delete file descriptor to epoll\n");
      //   close(epoll_fd);
      //   close(gfd);
      //   return 5;
      // }

      // if (close(epoll_fd)) {
      //   fprintf(stderr, "Failed to close epoll file descriptor\n");
      //   return 6;
      // }
    }
  }

  int _event_cleanup(int channel) {}

  //----------------------------------

  int add_edge_detect(int channel, Edge edge, uint64_t bounce_time = 0)
  {
    int result;

    // Enter Mutex
    _epmutex.lock();

    if (!_epoll_fd_thread) {
      _epoll_fd_thread = new std::thread(epoll_thread_callable);
    }

    // auto geo;
    std::shared_ptr<GPIOEventObject> geo;
    auto find_result = _gpio_events.find(channel);
    if (find_result != _gpio_events.end()) {
      geo = find_result->second;

      if (geo->edge != edge) {
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
      geo = std::make_shared<GPIOEventObject>();
      geo->channel = channel;
      geo->edge = edge;
      geo->bounce_time = bounce_time;

      // Set Event
      result = _write_sysfs_edge(channel, edge);
      if (result) {
        _epmutex.unlock();
        return result;
      }

      // Open the value fd
      geo->fd = _open_sysfd_value(channel);

      // Set
      _gpio_events[channel] = geo;
    }
    _epmutex.unlock();
  }

  void remove_edge_detect(int channel)
  {
    // Enter Mutex
    _epmutex.lock();

    auto geo;
    auto find_result = _gpio_events.find(channel);
    if (find_result != _gpio_events.end()) {
      geo = find_result->second;

      if (geo.edge != edge) {
        // Cannot have conflicting event types for a single channel
        _epmutex.unlock();
        return -1;
      }
      else if (!bounce_time && geo.bounce_time != bounce_time) {
        // Cannot have multiple conflicting bounce_times for a single channel
        _epmutex.unlock();
        return -2;
      }
    }
    else {
    }
  }

  void add_edge_callback(int channel, void (*callback)(int, Edge));
  void remove_edge_callback(int channel, void (*callback)(int, Edge));

  void wait_for_edge(int channel, Edge edge, unsigned long bounce_time = 0, unsigned long timeout = 0) {}
}