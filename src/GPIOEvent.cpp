/*
Copyright (c) 2012-2017 Ben Croston ben@croston.org.
Copyright (c) 2019, NVIDIA CORPORATION.
Copyright (c) 2019 Jueon Park(pjueon) bluegbg@gmail.com.
Copyright (c) 2021 Adam Rasburn blackforestcheesecake@protonmail.ch

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/

#include "private/GPIOEvent.h"
#include "private/PythonFunctions.h"
#include "private/SysfsRoot.h"

#include <fcntl.h>
#include <sys/epoll.h>
#include <time.h>
#include <unistd.h>

#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <deque>
#include <iostream>
#include <map>
#include <mutex>
#include <thread>
#include <vector>
#include <memory>


constexpr size_t MAX_EPOLL_EVENTS = 20;

namespace GPIO
{
    // Error messages for all errors produced by this module
    std::map<EventResultCode, const char*> event_error_code_to_message = {
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
        {EventResultCode::ConflictingBounceTime,
         "Already opened channel is currently employing a different bounce time"},
        {EventResultCode::InternalTrackingError, "Internal Event Tracking Error"},
        {EventResultCode::EpollFD_CreateError, "Failed to create the EPOLL file descriptor"},
        {EventResultCode::EpollCTL_Add, "Failure to add an event to the EPOLL file descriptor"},
        {EventResultCode::EpollWait, "Error occurred during call to epoll_wait"},
        {EventResultCode::GPIO_Event_Not_Found,
         "A channel event was not added to add a callback to. Call add_event_detect() first"},
    };

    struct _gpioEventObject
    {
        enum ModifyEvent
        {
            NONE,
            ADD,
            INITIAL_ABSCOND,
            REMOVE,
            MODIFY
        } _epoll_change_flag;
        struct epoll_event _epoll_event;

        std::string channel_id;
        int gpio;
        int fd;
        Edge edge;
        uint64_t bounce_time;
        uint64_t last_event;

        bool event_occurred;

        bool blocking_usage, concurrent_usage;
        std::vector<Callback> callbacks;
    };

    std::recursive_mutex _epmutex;
    std::unique_ptr<std::thread> _epoll_fd_thread = nullptr;
    std::atomic_bool _epoll_run_loop;

    std::map<int, std::shared_ptr<_gpioEventObject>> _gpio_events;
    std::atomic_int _auth_event_channel_count(0);
    std::map<int, int> _fd_to_gpio_map;

    //----------------------------------

    int _write_sysfs_edge(const std::string gpio_name, Edge edge, bool allow_none = true)
    {
        auto buf = format("%s/%s/edge", _SYSFS_ROOT, gpio_name.c_str());

        int edge_fd = open(buf.c_str(), O_WRONLY);
        if (edge_fd == -1)
        {
            // I/O Error
            std::perror("sysfs/edge open");
            return (int)GPIO::EventResultCode::SysFD_EdgeOpen;
        }

        auto get_result = [=]() -> int
        {
            switch (edge)
            {
            case Edge::RISING:
                return write(edge_fd, "rising", 7);
            case Edge::FALLING:
                return write(edge_fd, "falling", 7);
            case Edge::BOTH:
                return write(edge_fd, "both", 7);
            case Edge::NONE:
            {
                if (!allow_none)
                {
                    return (int)GPIO::EventResultCode::UnallowedEdgeNone;
                }
                return write(edge_fd, "none", 7);
            }
            case Edge::UNKNOWN:

            default:
                std::cerr << format("Bad argument, edge=%i\n", (int)edge);
                return (int)GPIO::EventResultCode::IllegalEdgeArgument;
            }
        };

        int result = get_result();

        if (result >= 0)
            result = 0;
        else if (result == -1)
        {
            // Print additional detail and label as a edge write error
            std::perror("sysfs/edge write");
            result = (int)GPIO::EventResultCode::SysFD_EdgeWrite;
        }

        close(edge_fd);
        return result;
    }

    int _open_sysfd_value(const std::string& gpio_name, int& fd)
    {
        auto buf = format("%s/%s/value", _SYSFS_ROOT, gpio_name.c_str());
        fd = open(buf.c_str(), O_RDONLY);

        if (fd == -1)
        {
            std::perror("sysfs/value open");
            return (int)GPIO::EventResultCode::SysFD_ValueOpen;
        }

        // Set the file descriptor to a non-blocking usage
        int result = fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
        if (result == -1)
        {
            std::perror("fcntl");
            close(fd);
            return (int)GPIO::EventResultCode::SysFD_ValueNonBlocking;
        }

        return 0;
    }

    std::map<int, std::shared_ptr<_gpioEventObject>>::iterator
    _epoll_thread_remove_event(int epoll_fd, std::map<int, std::shared_ptr<_gpioEventObject>>::iterator geo_it)
    {
        auto geo = geo_it->second;

        if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, geo->fd, 0) == -1)
        {
            // Okay to ignore I believe. File will be closed just below anyways.
        }

        auto fg_it = _fd_to_gpio_map.find(geo->fd);
        if (fg_it != _fd_to_gpio_map.end())
            _fd_to_gpio_map.erase(fg_it);

        // Close the fd
        if (close(geo->fd) == -1)
        {
            std::cerr << "[WARNING] Failed to close Epoll_Thread file descriptor\n";
        }

        // Erase from the map collection
        return _gpio_events.erase(geo_it);
    }

    void _epoll_thread_loop()
    {
        int epoll_fd = epoll_create1(0);
        if (epoll_fd == -1)
        {
            std::perror("[Fatal Error] Failed to create epoll file descriptor for concurrent Epoll Thread\n");
            return;
        }

        auto cleanup_and_return = [&epoll_fd]()
        {
            // Cleanup - thread is ending
            // -- GPIO Event Objects
            {
                std::lock_guard<std::recursive_mutex> mutex_lock(_epmutex);
                for (auto geo_it = _gpio_events.begin(); geo_it != _gpio_events.end();)
                {
                    geo_it = _epoll_thread_remove_event(epoll_fd, geo_it);
                }
            }

            // epoll
            if (close(epoll_fd) == -1)
            {
                std::perror(
                    "[WARNING] Failed to close epoll file descriptor during closure of concurrent Epoll_Thread\n");
            }
        };

        epoll_event events[MAX_EPOLL_EVENTS]{};
        while (_epoll_run_loop)
        {
            // Wait a small time for events
            int event_count = epoll_wait(epoll_fd, events, MAX_EPOLL_EVENTS, 1);
            std::lock_guard<std::recursive_mutex> mutex_lock(_epmutex);

            // Handle Events
            if (event_count)
            {
                if (event_count < 0)
                {
                    if (errno != EINTR)
                    {
                        std::perror("[Fatal Error] epoll_wait");
                        return cleanup_and_return();
                    }
                    break;
                }

                // Handle event modifications
                auto tick = (uint64_t)std::chrono::duration_cast<std::chrono::milliseconds>(
                                std::chrono::system_clock::now().time_since_epoch())
                                .count();

                // Iterate through each collected event
                for (int e = 0; e < event_count; e++)
                {
                    // Obtain the event object for the event
                    auto gpio_it = _fd_to_gpio_map.find(events[e].data.fd);
                    if (gpio_it == _fd_to_gpio_map.end())
                    {
                        // Shouldn't happen - ignore it if it does
                        continue;
                    }

                    auto geo_it = _gpio_events.find(gpio_it->second);
                    if (geo_it == _gpio_events.end())
                    {
                        // Shouldn't happen
                        // If it does -- ensure the fd is deleted & ignore any errors
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[e].data.fd, 0);

                        // Remove the item from the map
                        _fd_to_gpio_map.erase(gpio_it);
                        continue;
                    }

                    if (geo_it->second->_epoll_change_flag != _gpioEventObject::ModifyEvent::NONE)
                    {
                        // To be dealt with later. No events should be fired in this case
                        continue;
                    }

                    // Event & GPIO
                    auto geo = geo_it->second;

                    // Check event filter conditions
                    if (geo->bounce_time)
                    {
                        if (tick - geo->last_event < geo->bounce_time)
                        {
                            continue;
                        }

                        geo->last_event = tick;
                    }

                    // Fire event
                    geo->event_occurred = true;
                    for (auto& cb : geo->callbacks)
                    {
                        cb(geo->channel_id);
                    }
                }
            }

            // Handle changes/modifications to GPIO event objects
            for (auto geo_it = _gpio_events.begin(); geo_it != _gpio_events.end();)
            {
                auto geo = geo_it->second;
                switch (geo->_epoll_change_flag)
                {
                case _gpioEventObject::ModifyEvent::NONE:
                {
                    // No change
                }
                break;
                case _gpioEventObject::ModifyEvent::INITIAL_ABSCOND:
                {
                    geo->_epoll_change_flag = _gpioEventObject::ModifyEvent::NONE;
                }
                break;
                case _gpioEventObject::ModifyEvent::MODIFY:
                {
                    // For now this just means modification of the edge type, which is done on the calling thread
                    // Just set back to NONE and continue
                    geo->_epoll_change_flag = _gpioEventObject::ModifyEvent::NONE;
                }
                break;
                case _gpioEventObject::ModifyEvent::ADD:
                {
                    geo->_epoll_event.events = EPOLLIN | EPOLLPRI | EPOLLET;
                    geo->_epoll_event.data.fd = geo->fd;

                    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, geo->fd, &geo->_epoll_event) == -1)
                    {
                        // Error - Leave loop immediately
                        std::perror("epoll_ctl()");

                        return cleanup_and_return();
                    }

                    // Avoid the initial event (that would have occurred before this unit has been added)
                    geo->_epoll_change_flag = _gpioEventObject::ModifyEvent::INITIAL_ABSCOND;
                }
                break;
                case _gpioEventObject::ModifyEvent::REMOVE:
                {
                    if (geo->blocking_usage)
                    {
                        // Do not remove it during a concurrent blocking usage
                        break;
                    }

                    geo_it = _epoll_thread_remove_event(epoll_fd, geo_it);

                    // Skip past the iteration so as to not iterate past the returned element
                    // -- (which is the next element)
                    continue;
                }
                }

                // Iterate to next element
                geo_it++;
            }
        }

        return cleanup_and_return();
    }

    void _epoll_start_thread()
    {
        _epoll_run_loop = true;
        _epoll_fd_thread = std::make_unique<std::thread>(_epoll_thread_loop);
    }

    void _epoll_end_thread()
    {
        _epoll_run_loop = false;

        // Wait to join
        _epoll_fd_thread->join();

        {
            // Enter Mutex and clear thread
            std::lock_guard<std::recursive_mutex> mutex_lock(_epmutex);
            _epoll_fd_thread = nullptr;
        }
    }

    //-------------- Operations -------------------- //

    int _blocking_wait_for_edge(int gpio, const std::string& gpio_name, const std::string& channel_id, Edge edge,
                                uint64_t bounce_time, uint64_t timeout)
    {
        timespec timeout_time{};
        if (timeout)
        {
            clock_gettime(CLOCK_REALTIME, &timeout_time);

            timeout_time.tv_nsec += (timeout % 1000) * 1e6;
            long overlap = timeout_time.tv_nsec / 1e9;
            timeout_time.tv_nsec -= overlap;

            timeout_time.tv_sec += timeout / 1000 + overlap;
        }

        int result{};
        std::shared_ptr<_gpioEventObject> geo{};

        {
            // Enter Mutex
            std::lock_guard<std::recursive_mutex> mutex_lock(_epmutex);

            // Ensure a conflict does not exist with the concurrent event detecting thread and its collection of gpio
            // events
            auto find_result = _gpio_events.find(gpio);
            if (find_result != _gpio_events.end())
            {
                geo = find_result->second;

                if (geo->blocking_usage)
                {
                    // Channel already being blocked (can only be by another thread call)
                    return (int)GPIO::EventResultCode::ChannelAlreadyBlocked;
                }

                switch (geo->_epoll_change_flag)
                {
                case _gpioEventObject::ModifyEvent::NONE:
                case _gpioEventObject::ModifyEvent::ADD:
                case _gpioEventObject::ModifyEvent::MODIFY:
                {
                    if (geo->edge != edge)
                    {
                        return (int)GPIO::EventResultCode::ConflictingEdgeType;
                    }
                    if (bounce_time && geo->bounce_time != bounce_time)
                    {
                        return (int)GPIO::EventResultCode::ConflictingBounceTime;
                    }
                }
                break;
                case _gpioEventObject::ModifyEvent::REMOVE:
                {
                    // The epoll thread is inbetween concurrent transactions. Modify the existing
                    // object instead of removing it

                    if (geo->edge != edge)
                    {
                        // Update
                        geo->_epoll_change_flag = _gpioEventObject::ModifyEvent::MODIFY;
                        geo->edge = edge;

                        // Set Event
                        result = _write_sysfs_edge(gpio_name, edge);
                        if (result)
                        {
                            return result;
                        }
                    }

                    // Reset GPIO
                    geo->event_occurred = false;
                    geo->bounce_time = bounce_time;
                    geo->last_event = 0;

                    ++_auth_event_channel_count;
                }
                break;
                default:
                    // Shouldn't happen
                    return (int)GPIO::EventResultCode::InternalTrackingError;
                }
            }
            else
            {
                // Create a gpio-event-object to avoid concurrent conflicts for the channel while blocking
                geo = std::make_shared<_gpioEventObject>();
                geo->_epoll_change_flag = _gpioEventObject::ModifyEvent::ADD;
                geo->gpio = gpio;
                geo->channel_id = channel_id;
                geo->edge = edge;
                geo->bounce_time = bounce_time;
                geo->last_event = 0;

                // Open the value fd
                result = _open_sysfd_value(gpio_name, geo->fd);
                if (result)
                {
                    return result;
                }

                // Set Event
                result = _write_sysfs_edge(gpio_name, edge);
                if (result)
                {
                    close(geo->fd);
                    return result;
                }

                // Set
                _fd_to_gpio_map[geo->fd] = gpio;
                _gpio_events[gpio] = geo;

                ++_auth_event_channel_count;
            }

            geo->blocking_usage = true;
        }

        // Execute the epoll awaiting the event
        int epoll_fd = 0, error = 0;

        auto cleanup_and_return_result = [&]() -> int
        {
            if (epoll_fd >= 0)
                close(epoll_fd);

            // GPIO Event Object Tidy-up
            {
                // Enter Mutex
                std::unique_lock<std::recursive_mutex> mutex_lock(_epmutex);
                geo->blocking_usage = false;
                if (!geo->concurrent_usage)
                {
                    // Remove it
                    if (geo->_epoll_change_flag == _gpioEventObject::ModifyEvent::ADD)
                    {
                        // It hasn't been added to the concurrent epoll-thread yet (if there even is one)
                        auto ftg_it = _fd_to_gpio_map.find(geo->fd);
                        if (ftg_it != _fd_to_gpio_map.end())
                            _fd_to_gpio_map.erase(ftg_it);

                        // Close the fd
                        if (close(geo->fd) == -1)
                        {
                            std::cerr << "[WARNING] Failed to close Epoll_Thread file descriptor\n";
                        }

                        auto geo_it = _gpio_events.find(gpio);
                        if (geo_it != _gpio_events.end())
                            _gpio_events.erase(geo_it);

                        --_auth_event_channel_count;
                        if (_auth_event_channel_count == 0 && _epoll_fd_thread)
                        {
                            // Signal shutdown of thread
                            // -- Doesn't need to run if there are no events
                            mutex_lock.unlock();
                            _epoll_end_thread();
                        }
                    }
                    else
                    {
                        // Set for removal from the concurrent epoll-thread
                        _remove_edge_detect(gpio);
                    }
                }
            }

            if (error)
                return error;
            return result;
        };

        result = (int)EventResultCode::None; // 0
        {
            epoll_fd = epoll_create1(0);
            if (epoll_fd == -1)
            {
                error = (int)GPIO::EventResultCode::EpollFD_CreateError;
                std::perror("epoll_create1(0)");
                return cleanup_and_return_result();
            }

            geo->_epoll_event.events = EPOLLIN | EPOLLPRI | EPOLLET;
            geo->_epoll_event.data.fd = geo->fd;

            if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, geo->fd, &geo->_epoll_event) == -1)
            {
                close(epoll_fd);
                std::perror("epoll_ctl():");
                error = (int)GPIO::EventResultCode::EpollCTL_Add;
                return cleanup_and_return_result();
            }

            epoll_event events[1]{};
            bool initial_edge = true;

            // Remove Initial Event
            timespec current_time{};
            while (true)
            {
                if (timeout)
                {
                    clock_gettime(CLOCK_REALTIME, &current_time);
                    if (current_time.tv_sec > timeout_time.tv_sec ||
                        (current_time.tv_sec == timeout_time.tv_sec && current_time.tv_nsec >= timeout_time.tv_nsec))
                    {
                        // Time-out
                        break;
                    }
                }

                // Wait a small time for events
                int event_count = epoll_wait(epoll_fd, events, 1, 1);

                // First trigger is with current state so ignore
                if (initial_edge)
                {
                    initial_edge = false;
                    continue;
                }

                // Handle Events
                if (event_count)
                {
                    if (event_count == -1)
                    {
                        if (errno != EINTR)
                        {
                            std::perror("epoll_wait");
                            error = (int)EventResultCode::EpollWait;
                        }
                        return cleanup_and_return_result();
                    }

                    if (events[0].data.fd == geo->fd)
                    {
                        auto tick = (uint64_t)std::chrono::duration_cast<std::chrono::milliseconds>(
                                        std::chrono::system_clock::now().time_since_epoch())
                                        .count();

                        if (!geo->bounce_time || tick - geo->last_event > geo->bounce_time)
                        {
                            result = (int)EventResultCode::EdgeDetected;
                            break;
                        }
                    }
                }
            }
        }

        return cleanup_and_return_result();
    }

    bool _edge_event_detected(int gpio)
    {
        bool result = false;

        // Enter Mutex
        std::lock_guard<std::recursive_mutex> mutex_lock(_epmutex);

        std::shared_ptr<_gpioEventObject> geo;
        auto find_result = _gpio_events.find(gpio);
        if (find_result != _gpio_events.end())
        {
            geo = find_result->second;
            result = geo->event_occurred;
            geo->event_occurred = false;
        }

        return result;
    }

    bool _edge_event_exists(int gpio)
    {
        std::lock_guard<std::recursive_mutex> mutex_lock(_epmutex);

        auto find_result = _gpio_events.find(gpio);
        if (find_result != _gpio_events.end())
        {
            return find_result->second->_epoll_change_flag != _gpioEventObject::ModifyEvent::REMOVE;
        }
        return false;
    }

    int _add_edge_detect(int gpio, const std::string& gpio_name, const std::string& channel_id, Edge edge,
                         uint64_t bounce_time)
    {
        int result{};

        // Enter Mutex
        std::lock_guard<std::recursive_mutex> mutex_lock(_epmutex);

        // auto geo;
        std::shared_ptr<_gpioEventObject> geo;
        auto find_result = _gpio_events.find(gpio);
        if (find_result != _gpio_events.end())
        {
            geo = find_result->second;

            switch (geo->_epoll_change_flag)
            {
            case _gpioEventObject::ModifyEvent::NONE:
            case _gpioEventObject::ModifyEvent::ADD:
            case _gpioEventObject::ModifyEvent::MODIFY:
            {
                if (geo->edge != edge)
                {
                    return (int)GPIO::EventResultCode::ConflictingEdgeType;
                }
                if (bounce_time && geo->bounce_time != bounce_time)
                {
                    return (int)GPIO::EventResultCode::ConflictingBounceTime;
                }

                // Otherwise, do nothing more
            }
            break;
            case _gpioEventObject::ModifyEvent::REMOVE:
            {
                // The epoll thread is inbetween concurrent transactions. Modify the existing
                // object instead of removing it

                if (geo->edge != edge)
                {
                    // Update
                    geo->_epoll_change_flag = _gpioEventObject::ModifyEvent::MODIFY;
                    geo->edge = edge;

                    // Set Event
                    result = _write_sysfs_edge(gpio_name, edge);
                    if (result)
                    {
                        return result;
                    }
                }
                geo->bounce_time = bounce_time;
                geo->last_event = 0;
                ++_auth_event_channel_count;
            }
            break;
            default:
                // Shouldn't happen
                return (int)GPIO::EventResultCode::InternalTrackingError;
            }
        }
        else
        {
            // Configure anew
            geo = std::make_shared<_gpioEventObject>();
            geo->_epoll_change_flag = _gpioEventObject::ModifyEvent::ADD;
            geo->gpio = gpio;
            geo->channel_id = channel_id;
            geo->edge = edge;
            geo->last_event = 0;
            geo->blocking_usage = false;
            geo->event_occurred = false;

            // Open the value fd
            result = _open_sysfd_value(gpio_name, geo->fd);
            if (result)
            {
                return result;
            }
            _fd_to_gpio_map[geo->fd] = gpio;

            // Set Event
            result = _write_sysfs_edge(gpio_name, edge);
            if (result)
            {
                close(geo->fd);
                return result;
            }

            // Set
            _gpio_events[gpio] = geo;
            ++_auth_event_channel_count;
        }

        geo->bounce_time = bounce_time;
        geo->concurrent_usage = true;

        if (!_epoll_fd_thread)
        {
            _epoll_start_thread();
        }

        return 0;
    }

    void _remove_edge_detect(int gpio)
    {
        // Enter Mutex
        std::unique_lock<std::recursive_mutex> mutex_lock(_epmutex);

        auto find_result = _gpio_events.find(gpio);
        if (find_result != _gpio_events.end())
        {
            auto geo = find_result->second;

            geo->_epoll_change_flag = _gpioEventObject::ModifyEvent::REMOVE;
            --_auth_event_channel_count;
            geo->concurrent_usage = false;
            if (geo->blocking_usage)
            {
                // Channel is currently in a blocking usage on a concurrent thread
                // -- At least remove all callbacks right now
                geo->callbacks.clear();
                return;
            }

            if (_auth_event_channel_count == 0 && _epoll_fd_thread)
            {
                // Signal shutdown of thread
                // -- Doesn't need to run if there are no events
                mutex_lock.unlock();
                _epoll_end_thread();
            }
        }
    }

    int _add_edge_callback(int gpio, const Callback& callback)
    {
        std::lock_guard<std::recursive_mutex> mutex_lock(_epmutex);

        auto find_result = _gpio_events.find(gpio);
        if (find_result == _gpio_events.end())
        {
            return (int)GPIO::EventResultCode::GPIO_Event_Not_Found;
        }

        auto geo = find_result->second;
        geo->callbacks.push_back(callback);

        return 0;
    }

    void _remove_edge_callback(int gpio, const Callback& callback)
    {
        std::lock_guard<std::recursive_mutex> mutex_lock(_epmutex);

        auto find_result = _gpio_events.find(gpio);
        if (find_result == _gpio_events.end())
        {
            // Couldn't find it, Doesn't matter.
            return;
        }

        auto geo = find_result->second;
        for (auto geo_it = geo->callbacks.begin(); geo_it != geo->callbacks.end();)
        {
            if (*geo_it == callback)
            {
                geo->callbacks.erase(geo_it);
            }
            else
            {
                geo_it++;
            }
        }
    }

    void _event_cleanup(int gpio, const std::string& gpio_name) { _remove_edge_detect(gpio); }

} // namespace GPIO
