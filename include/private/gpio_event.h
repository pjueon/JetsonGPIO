/*
Copyright (c) 2019, NVIDIA CORPORATION.
Copyright (c) 2019 Jueon Park(pjueon) bluegbg@gmail.com.

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
#ifndef GPIO_EVENT
#define GPIO_EVENT

// #include <string>
#include <map>
// #include <vector>

#include "JetsonGPIO.h"
// #include "private/Model.h"

namespace GPIO {
enum class EventErrorCode {
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
  GPIO_Event_Not_Found = -112,
  None = 0,
};

std::map<EventErrorCode, const char *> event_error_msg = {
    {EventErrorCode::SysFD_EdgeOpen, "Failure to open the /sys/class/gpio/gpio{$ch}/edge file"},
    {EventErrorCode::UnallowedEdgeNone, "Specifying Edge as 'none' was not allowed"},
    {EventErrorCode::IllegalEdgeArgument, "Illegal Edge argument"},
    {EventErrorCode::SysFD_EdgeWrite, "Failure to write to the /sys/class/gpio/gpio{$ch}/edge file"},
    {EventErrorCode::SysFD_ValueOpen, "Failure to open the channels System value file descriptor"},
    {EventErrorCode::SysFD_ValueNonBlocking,
     "Failure to set to non-blocking the channels System value file descriptor"},
    {EventErrorCode::ChannelAlreadyBlocked,
     "This channel is already being blocked (Probably by a concurrent wait_for_edge call)"},
    {EventErrorCode::ConflictingEdgeType, "Already opened channel is currently detecting a different edge type"},
    {EventErrorCode::ConflictingBounceTime, "Already opened channel is currently employing a different bounce time"},
    {EventErrorCode::InternalTrackingError, "Internal Event Tracking Error"},
    {EventErrorCode::EpollFD_CreateError, "Failed to create the EPOLL file descriptor"},
    {EventErrorCode::EpollCTL_Add, "Failure to add an event to the EPOLL file descriptor"},
    {EventErrorCode::GPIO_Event_Not_Found,
     "A channel event was not added to add a callback to. Call add_event_detect() first"},
};

int blocking_wait_for_edge(int gpio, int channel_id, Edge edge, uint64_t bounce_time, uint64_t timeout);

bool edge_event_detected(int gpio);
int add_edge_detect(int gpio, int channel_id, Edge edge, uint64_t bounce_time);
void remove_edge_detect(int gpio);
int add_edge_callback(int gpio, void (*callback)(int));
void remove_edge_callback(int gpio, void (*callback)(int));
void _event_cleanup(int gpio);
}  // namespace GPIO

#endif /* GPIO_EVENT */
