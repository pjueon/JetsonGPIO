
#include <mutex>
#include <thread>
#include <sys/epoll.h>

#include "JetsonGPIO.h"

namespace GPIO
{
    std::mutex epmutex;
    std::thread *_epoll_fd_thread = nullptr;

    typedef struct _gpioEventObject {
        
    } gpio_event_object;

    std::map<

   //----------------------------------

   void blocking_wait_for_edge();

   void epoll_thread() {

   }

   //----------------------------------

   int add_edge_detect(int channel, Edge edge, unsigned long bounce_time) {

       epmutex.lock();
       if(!_epoll_fd_thread) {
           _epoll_fd_thread = new std::thread();
       }
       epmutex.unlock();




    gpios = None
    res = gpio_event_added(gpio)

    # event not added
    if not res:
        gpios = _Gpios(gpio_name, edge, bouncetime)
        _set_edge(gpio_name, edge)

    # event already added
    elif res == edge:
        gpios = _get_gpio_object(gpio)
        if ((bouncetime is not None and gpios.bouncetime != bouncetime) or
                gpios.thread_added):
            return 1
    else:
        return 1
    # create epoll object for fd if not already open
    if _epoll_fd_thread is None:
        _epoll_fd_thread = epoll()
        if _epoll_fd_thread is None:
            return 2

    # add eventmask and fd to epoll object
    try:
        _epoll_fd_thread.register(gpios.value_fd, EPOLLIN | EPOLLET | EPOLLPRI)
    except IOError:
        remove_edge_detect(gpio, gpio_name)
        return 2

    gpios.thread_added = 1
    _gpio_event_list[gpio] = gpios

    # create and start poll thread if not already running
    if not _thread_running:
        try:
            thread.start_new_thread(_poll_thread, ())
        except RuntimeError:
            remove_edge_detect(gpio, gpio_name)
            return 2
    return 0;
   }

void remove_edge_detect(gpio, gpio_name) {
    if gpio not in _gpio_event_list:
        return

    if _epoll_fd_thread is not None:
        _epoll_fd_thread.unregister(_gpio_event_list[gpio].value_fd)

    _set_edge(gpio_name, NO_EDGE)

    _mutex.acquire()
    del _gpio_event_list[gpio]
    _mutex.release()
}

   void add_edge_callback(int channel, void (*callback)(int, Edge));
   void remove_edge_callback(int channel, void (*callback)(int, Edge));

   void wait_for_edge(int channel, Edge edge, unsigned long bounce_time = 0, unsigned long timeout = 0) {
   }
}