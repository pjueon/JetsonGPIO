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

#include <algorithm>
#include <cctype>
#include <chrono>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "JetsonGPIO.h"
#include "private/ExceptionHandling.h"
#include "private/GPIOEvent.h"
#include "private/GPIOPinData.h"
#include "private/MainModule.h"
#include "private/Model.h"
#include "private/ModelUtility.h"
#include "private/PythonFunctions.h"
#include "private/SysfsRoot.h"

using namespace GPIO;
using namespace std;

// APIs

LazyString GPIO::model{[]() { return global().model(); }};
LazyString GPIO::JETSON_INFO{[]() { return global().JETSON_INFO(); }};

void GPIO::setwarnings(bool state) { global()._gpio_warnings = state; }

void GPIO::setmode(NumberingModes mode)
{
    try
    {
        // check if mode is valid
        if (mode == NumberingModes::None)
            throw runtime_error("Pin numbering mode must be GPIO::BOARD, GPIO::BCM, GPIO::TEGRA_SOC or GPIO::CVM");
        // check if a different mode has been set
        if (global()._gpio_mode != NumberingModes::None && mode != global()._gpio_mode)
            throw runtime_error("A different mode has already been set!");

        global()._channel_data = global()._channel_data_by_mode.at(mode);
        global()._gpio_mode = mode;
    }
    catch (exception& e)
    {
        throw _error(e, "GPIO::setmode()");
    }
}

NumberingModes GPIO::getmode() { return global()._gpio_mode; }

void GPIO::setup(const string& channel, Directions direction, int initial)
{
    try
    {
        ChannelInfo ch_info = global()._channel_to_info(channel, true);

        if (global()._gpio_warnings)
        {
            Directions sysfs_cfg = global()._sysfs_channel_configuration(ch_info);
            Directions app_cfg = global()._app_channel_configuration(ch_info);

            if (app_cfg == UNKNOWN && sysfs_cfg != UNKNOWN)
            {
                cerr << "[WARNING] This channel is already in use, continuing anyway. Use GPIO::setwarnings(false) to "
                        "disable warnings. channel: "
                     << channel << endl;
            }
        }

        if (is_in(ch_info.channel, global()._channel_configuration))
            global()._cleanup_one(ch_info);

        if (direction == OUT)
        {
            global()._setup_single_out(ch_info, initial);
        }
        else if (direction == IN)
        {
            if (!is_None(initial))
                throw runtime_error("initial parameter is not valid for inputs");
            global()._setup_single_in(ch_info);
        }
        else
            throw runtime_error("GPIO direction must be GPIO::IN or GPIO::OUT");
    }
    catch (exception& e)
    {
        throw _error(e, "GPIO::setup()");
    }
}

void GPIO::setup(int channel, Directions direction, int initial) { setup(to_string(channel), direction, initial); }

void GPIO::cleanup(const string& channel)
{
    try
    {
        // warn if no channel is setup
        if (global()._gpio_mode == NumberingModes::None && global()._gpio_warnings)
        {
            cerr << "[WARNING] No channels have been set up yet - nothing to clean up! "
                    "Try cleaning up at the end of your program instead!"
                 << endl;
            return;
        }

        // clean all channels if no channel param provided
        if (is_None(channel))
        {
            global()._cleanup_all();
            return;
        }

        ChannelInfo ch_info = global()._channel_to_info(channel);
        if (is_in(ch_info.channel, global()._channel_configuration))
        {
            global()._cleanup_one(ch_info);
        }
    }
    catch (exception& e)
    {
        throw _error(e, "GPIO::cleanup()");
    }
}

void GPIO::cleanup(int channel)
{
    string str_channel = to_string(channel);
    cleanup(str_channel);
}

int GPIO::input(const string& channel)
{
    try
    {
        ChannelInfo ch_info = global()._channel_to_info(channel, true);

        Directions app_cfg = global()._app_channel_configuration(ch_info);

        if (app_cfg != IN && app_cfg != OUT)
            throw runtime_error("You must setup() the GPIO channel first");

        ch_info.f_value->seekg(0, std::ios::beg);
        int value_read{};
        *ch_info.f_value >> value_read;
        return value_read;
    }
    catch (exception& e)
    {
        throw _error(e, "GPIO::input()");
    }
}

int GPIO::input(int channel) { return input(to_string(channel)); }

/* Function used to set a value to a channel.
   Values must be either HIGH or LOW */
void GPIO::output(const string& channel, int value)
{
    try
    {
        ChannelInfo ch_info = global()._channel_to_info(channel, true);
        // check that the channel has been set as output
        if (global()._app_channel_configuration(ch_info) != OUT)
            throw runtime_error("The GPIO channel has not been set up as an OUTPUT");
        global()._output_one(ch_info, value);
    }
    catch (exception& e)
    {
        throw _error(e, "GPIO::output()");
    }
}

void GPIO::output(int channel, int value) { output(to_string(channel), value); }

Directions GPIO::gpio_function(const string& channel)
{
    try
    {
        ChannelInfo ch_info = global()._channel_to_info(channel);
        return global()._sysfs_channel_configuration(ch_info);
    }
    catch (exception& e)
    {
        throw _error(e, "GPIO::gpio_function()");
    }
}

Directions GPIO::gpio_function(int channel) { return gpio_function(to_string(channel)); }

//=============================== Events =================================

bool GPIO::event_detected(const std::string& channel)
{
    ChannelInfo ch_info = global()._channel_to_info(channel, true);
    try
    {
        // channel must be setup as input
        Directions app_cfg = global()._app_channel_configuration(ch_info);
        if (app_cfg != Directions::IN)
            throw runtime_error("You must setup() the GPIO channel as an input first");

        return _edge_event_detected(ch_info.gpio);
    }
    catch (exception& e)
    {
        throw _error(e, "GPIO::event_detected()");
    }
}

bool GPIO::event_detected(int channel) { return event_detected(std::to_string(channel)); }

void GPIO::add_event_callback(const std::string& channel, const Callback& callback)
{
    try
    {
        // Argument Check
        if (callback == nullptr)
        {
            throw invalid_argument("callback cannot be null");
        }

        ChannelInfo ch_info = global()._channel_to_info(channel, true);

        // channel must be setup as input
        Directions app_cfg = global()._app_channel_configuration(ch_info);
        if (app_cfg != Directions::IN)
        {
            throw runtime_error("You must setup() the GPIO channel as an input first");
        }

        // edge event must already exist
        if (!_edge_event_exists(ch_info.gpio))
            throw runtime_error("The edge event must have been set via add_event_detect()");

        // Execute
        EventResultCode result = (EventResultCode)_add_edge_callback(ch_info.gpio, callback);
        switch (result)
        {
        case EventResultCode::None:
            break;
        default:
        {
            const char* error_msg = event_error_code_to_message[result];
            throw runtime_error(error_msg ? error_msg : "Unknown Error");
        }
        }
    }
    catch (exception& e)
    {
        throw _error(e, "GPIO::add_event_callback()");
    }
}

void GPIO::add_event_callback(int channel, const Callback& callback)
{
    add_event_callback(std::to_string(channel), callback);
}

void GPIO::remove_event_callback(const std::string& channel, const Callback& callback)
{
    ChannelInfo ch_info = global()._channel_to_info(channel, true);

    _remove_edge_callback(ch_info.gpio, callback);
}

void GPIO::remove_event_callback(int channel, const Callback& callback)
{
    remove_event_callback(std::to_string(channel), callback);
}

void GPIO::add_event_detect(const std::string& channel, Edge edge, const Callback& callback, unsigned long bounce_time)
{
    try
    {
        ChannelInfo ch_info = global()._channel_to_info(channel, true);

        // channel must be setup as input
        Directions app_cfg = global()._app_channel_configuration(ch_info);
        if (app_cfg != Directions::IN)
        {
            throw runtime_error("You must setup() the GPIO channel as an input first");
        }

        // edge provided must be rising, falling or both
        if (edge != Edge::RISING && edge != Edge::FALLING && edge != Edge::BOTH)
            throw invalid_argument("argument 'edge' must be set to RISING, FALLING or BOTH");

        // Execute
        EventResultCode result =
            (EventResultCode)_add_edge_detect(ch_info.gpio, ch_info.gpio_name, channel, edge, bounce_time);
        switch (result)
        {
        case EventResultCode::None:
            break;
        default:
        {
            const char* error_msg = event_error_code_to_message[result];
            throw runtime_error(error_msg ? error_msg : "Unknown Error");
        }
        }

        if (callback != nullptr)
        {
            if (_add_edge_callback(ch_info.gpio, callback))
                // Shouldn't happen (--it was just added successfully)
                throw runtime_error("Couldn't add callback due to unknown error with just added event");
        }
    }
    catch (exception& e)
    {
        throw _error(e, "GPIO::add_event_detect()");
    }
}

void GPIO::add_event_detect(int channel, Edge edge, const Callback& callback, unsigned long bounce_time)
{
    return add_event_detect(std::to_string(channel), edge, callback, bounce_time);
}

void GPIO::remove_event_detect(const std::string& channel)
{
    ChannelInfo ch_info = global()._channel_to_info(channel, true);

    _remove_edge_detect(ch_info.gpio);
}

void GPIO::remove_event_detect(int channel) { remove_event_detect(std::to_string(channel)); }

WaitResult GPIO::wait_for_edge(const std::string& channel, Edge edge, uint64_t bounce_time, uint64_t timeout)
{
    try
    {
        ChannelInfo ch_info = global()._channel_to_info(channel, true);

        // channel must be setup as input
        Directions app_cfg = global()._app_channel_configuration(ch_info);
        if (app_cfg != Directions::IN)
        {
            throw runtime_error("You must setup() the GPIO channel as an input first");
        }

        // edge provided must be rising, falling or both
        if (edge != Edge::RISING && edge != Edge::FALLING && edge != Edge::BOTH)
            throw invalid_argument("argument 'edge' must be set to RISING, FALLING or BOTH");

        // Execute
        EventResultCode result = (EventResultCode)_blocking_wait_for_edge(ch_info.gpio, ch_info.gpio_name, channel,
                                                                          edge, bounce_time, timeout);
        switch (result)
        {
        case EventResultCode::None:
            // Timeout
            return (std::string)None;
        case EventResultCode::EdgeDetected:
            // Event Detected
            return channel;
        default:
        {
            const char* error_msg = event_error_code_to_message[result];
            throw runtime_error(error_msg ? error_msg : "Unknown Error");
        }
        }
    }
    catch (exception& e)
    {
        throw _error(e, "GPIO::wait_for_edge()");
    }
}

WaitResult GPIO::wait_for_edge(int channel, Edge edge, uint64_t bounce_time, uint64_t timeout)
{
    return wait_for_edge(std::to_string(channel), edge, bounce_time, timeout);
}