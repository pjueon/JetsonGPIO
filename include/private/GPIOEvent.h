/*
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

#pragma once
#ifndef GPIO_EVENT_H
#define GPIO_EVENT_H

#include "JetsonGPIO/Callback.h"
#include "JetsonGPIO/PublicEnums.h"
#include <map>
#include <string>

namespace GPIO
{
    enum class EventResultCode
    {
        SysFD_EdgeOpen = -100,
        UnallowedEdgeNone = -101,
        IllegalEdgeArgument = -102,
        SysFD_EdgeWrite = -103,
        SysFD_ValueOpen = -104,
        SysFD_ValueNonBlocking = -105,
        ChannelAlreadyBlocked = -106,
        ConflictingEdgeType = -107,
        ConflictingBounceTime = -108,
        InternalTrackingError = -109,
        EpollFD_CreateError = -110,
        EpollCTL_Add = -111,
        EpollWait = -112,
        GPIO_Event_Not_Found = -113,
        None = 0,
        EdgeDetected = 1,
    };

    extern std::map<EventResultCode, const char*> event_error_code_to_message;

    int _blocking_wait_for_edge(int gpio, const std::string& gpio_name, const std::string& channel_id, Edge edge,
                                uint64_t bounce_time, uint64_t timeout);

    bool _edge_event_detected(int gpio);
    bool _edge_event_exists(int gpio);

    int _add_edge_detect(int gpio, const std::string& gpio_name, const std::string& channel_id, Edge edge,
                         uint64_t bounce_time);
    void _remove_edge_detect(int gpio);

    int _add_edge_callback(int gpio, const Callback& callback);
    void _remove_edge_callback(int gpio, const Callback& callback);

    void _event_cleanup(int gpio, const std::string& gpio_name);
} // namespace GPIO

#endif
