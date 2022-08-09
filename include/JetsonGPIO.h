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

#pragma once
#ifndef JETSON_GPIO_H
#define JETSON_GPIO_H

#include <initializer_list>
#include <string>
#include <vector>

#include "JetsonGPIO/Callback.h"
#include "JetsonGPIO/LazyString.h"
#include "JetsonGPIO/PWM.h"
#include "JetsonGPIO/PublicEnums.h"
#include "JetsonGPIO/TypeTraits.h"
#include "JetsonGPIO/WaitResult.h"
#include "JetsonGPIOConfig.h"

namespace GPIO
{
    constexpr auto VERSION = JETSONGPIO_VERSION;

    extern LazyString JETSON_INFO;
    extern LazyString model;

    constexpr int HIGH = 1;
    constexpr int LOW = 0;

    // Function used to enable/disable warnings during setup and cleanup.
    void setwarnings(bool state);

    /* Function used to set the pin mumbering mode.
       @mode must be one of BOARD, BCM, TEGRA_SOC or CV */
    void setmode(NumberingModes mode);

    // Function used to get the currently set pin numbering mode
    NumberingModes getmode();

    /* Function used to setup individual pins as Input or Output.
       @direction must be IN or OUT
       @initial must be HIGH, LOW or -1 and is only valid when direction is OUT  */
    void setup(const std::string& channel, Directions direction, int initial = -1);
    void setup(int channel, Directions direction, int initial = -1);

    /* Function used to cleanup channels at the end of the program.
       If no channel is provided, all channels are cleaned */
    void cleanup();
    void cleanup(const std::string& channel);
    void cleanup(int channel);
    void cleanup(const std::vector<int>& channels);
    void cleanup(const std::vector<std::string>& channels);
    void cleanup(const std::initializer_list<int>& channels);
    void cleanup(const std::initializer_list<std::string>& channels);

    /* Function used to return the current value of the specified channel.
       @returns either HIGH or LOW */
    int input(const std::string& channel);
    int input(int channel);

    /* Function used to set a value to a channel.
       @value must be either HIGH or LOW */
    void output(const std::string& channel, int value);
    void output(int channel, int value);

    /* Function used to check the currently set function of the channel specified. */
    Directions gpio_function(const std::string& channel);
    Directions gpio_function(int channel);

    /* Function used to check if an event occurred on the specified channel.
       Param channel must be an integer or a string.
       This function return True or False */
    bool event_detected(const std::string& channel);
    bool event_detected(int channel);

    /* Function used to add a callback function to channel, after it has been
       registered for events using add_event_detect() */
    void add_event_callback(const std::string& channel, const Callback& callback);
    void add_event_callback(int channel, const Callback& callback);

    /* Function used to remove a callback function previously added to detect a channel event */
    void remove_event_callback(const std::string& channel, const Callback& callback);
    void remove_event_callback(int channel, const Callback& callback);

    /* Function used to add threaded event detection for a specified gpio channel.
       @channel is an integer or a string specifying the channel
       @edge must be a member of GPIO::Edge
       @callback (optional) may be a callback function to be called when the event is detected (or nullptr)
       @bouncetime (optional) a button-bounce signal ignore time (in milliseconds, default=none) */
    void add_event_detect(const std::string& channel, Edge edge, const Callback& callback = nullptr,
                          unsigned long bounce_time = 0);
    void add_event_detect(int channel, Edge edge, const Callback& callback = nullptr, unsigned long bounce_time = 0);

    /* Function used to remove event detection for channel */
    void remove_event_detect(const std::string& channel);
    void remove_event_detect(int channel);

    /* Function used to perform a blocking wait until the specified edge event is detected within the specified
       timeout period. Returns the channel if an event is detected or 0 if a timeout has occurred.
       @channel is an integer or a string specifying the channel
       @edge must be a member of GPIO::Edge
       @bouncetime in milliseconds (optional)
       @timeout in milliseconds (optional)
       @returns WaitResult object */
    WaitResult wait_for_edge(const std::string& channel, Edge edge, unsigned long bounce_time = 0,
                             unsigned long timeout = 0);
    WaitResult wait_for_edge(int channel, Edge edge, unsigned long bounce_time = 0, unsigned long timeout = 0);

} // namespace GPIO

#endif // JETSON_GPIO_H
