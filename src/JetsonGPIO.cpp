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
#include <thread>
#include <unistd.h>
#include <utility>
#include <vector>

#include "JetsonGPIO.h"
#include "private/ExceptionHandling.h"
#include "private/Model.h"
#include "private/ModelUtility.h"
#include "private/PythonFunctions.h"
#include "private/GPIOEvent.h"
#include "private/GPIOPinData.h"
#include "private/SysfsRoot.h"

using namespace GPIO;
using namespace std;

// The user CAN'T use GPIO::UNKNOW, GPIO::HARD_PWM
// These are only for implementation
constexpr Directions UNKNOWN = Directions::UNKNOWN;
constexpr Directions HARD_PWM = Directions::HARD_PWM;

//================================================================================
// All global variables are wrapped in a singleton class except for public APIs,
// in order to avoid initialization order problem among global variables in
// different compilation units.

class MainModule
{
    // NOTE: DON'T change the declaration order of fields.
    // declaration order == initialization order
private:
    PinData _pinData;
    std::string _model;
    std::string _JETSON_INFO;
    string _gpio_dir(const ChannelInfo& ch_info) { return format("%s/%s", _SYSFS_ROOT, ch_info.gpio_name.c_str()); }

public:
    const map<GPIO::NumberingModes, map<string, ChannelInfo>> _channel_data_by_mode;

    // A map used as lookup tables for pin to linux gpio mapping
    map<string, ChannelInfo> _channel_data;

    bool _gpio_warnings;
    NumberingModes _gpio_mode;
    map<string, Directions> _channel_configuration;

    MainModule(const MainModule&) = delete;
    MainModule& operator=(const MainModule&) = delete;

    ~MainModule()
    {
        try
        {
            // In case the user forgot to call cleanup() at the end of the program.
            _cleanup_all();
        }
        catch (exception& e)
        {
            cerr << _error_message(e, "~_cleaner()") << std::endl;
        }
    }

    static MainModule& get_instance()
    {
        static MainModule singleton{};
        return singleton;
    }

    void _validate_mode_set()
    {
        if (_gpio_mode == NumberingModes::None)
            throw runtime_error(
                "Please set pin numbering mode using "
                "GPIO::setmode(GPIO::BOARD), GPIO::setmode(GPIO::BCM), GPIO::setmode(GPIO::TEGRA_SOC) or "
                "GPIO::setmode(GPIO::CVM)");
    }

    ChannelInfo _channel_to_info_lookup(const string& channel, bool need_gpio, bool need_pwm)
    {
        if (!is_in(channel, _channel_data))
            throw runtime_error("Channel " + channel + " is invalid");
        ChannelInfo ch_info = _channel_data.at(channel);
        if (need_gpio && is_None(ch_info.gpio_chip_dir))
            throw runtime_error("Channel " + channel + " is not a GPIO");
        if (need_pwm && is_None(ch_info.pwm_chip_dir))
            throw runtime_error("Channel " + channel + " is not a PWM");
        return ch_info;
    }

    ChannelInfo _channel_to_info(const string& channel, bool need_gpio = false, bool need_pwm = false)
    {
        _validate_mode_set();
        return _channel_to_info_lookup(channel, need_gpio, need_pwm);
    }

    vector<ChannelInfo> _channels_to_infos(const vector<string>& channels, bool need_gpio = false,
                                           bool need_pwm = false)
    {
        _validate_mode_set();
        vector<ChannelInfo> ch_infos{};
        for (const auto& c : channels)
        {
            ch_infos.push_back(_channel_to_info_lookup(c, need_gpio, need_pwm));
        }
        return ch_infos;
    }

    /* Return the current configuration of a channel as reported by sysfs.
       Any of IN, OUT, HARD_PWM, or UNKNOWN may be returned. */
    Directions _sysfs_channel_configuration(const ChannelInfo& ch_info)
    {
        if (!is_None(ch_info.pwm_chip_dir))
        {
            string pwm_dir = format("%s/pwm%i", ch_info.pwm_chip_dir.c_str(), ch_info.pwm_id);
            if (os_path_exists(pwm_dir))
                return HARD_PWM;
        }

        string gpio_dir = _gpio_dir(ch_info);
        if (!os_path_exists(gpio_dir))
            return UNKNOWN; // Originally returns None in NVIDIA's GPIO Python Library

        string gpio_direction{};
        { // scope for f
            ifstream f_direction(format("%s/direction", gpio_dir.c_str()));

            gpio_direction = read(f_direction);
            gpio_direction = lower(strip(gpio_direction));
        } // scope ends

        if (gpio_direction == "in")
            return IN;
        else if (gpio_direction == "out")
            return OUT;
        else
            return UNKNOWN; // Originally returns None in NVIDIA's GPIO Python Library
    }

    /* Return the current configuration of a channel as requested by this
       module in this process. Any of IN, OUT, or UNKNOWN may be returned. */
    Directions _app_channel_configuration(const ChannelInfo& ch_info)
    {
        if (!is_in(ch_info.channel, _channel_configuration))
            return UNKNOWN; // Originally returns None in NVIDIA's GPIO Python Library
        return _channel_configuration[ch_info.channel];
    }

    void _export_gpio(const ChannelInfo& ch_info)
    {
        string gpio_dir = _gpio_dir(ch_info);

        if (!os_path_exists(gpio_dir))
        { // scope for f_export
            ofstream f_export(_export_dir());
            f_export << ch_info.gpio;
        } // scope ends

        string value_path = format("%s/value", gpio_dir.c_str());

        int time_count = 0;
        while (!os_access(value_path, R_OK | W_OK))
        {
            this_thread::sleep_for(chrono::milliseconds(10));
            if (time_count++ > 100)
                throw runtime_error("Permission denied: path: " + value_path +
                                    "\n Please configure permissions or use the root user to run this.");
        }

        ch_info.f_direction->open(format("%s/direction", gpio_dir.c_str()), std::ios::out);
        ch_info.f_value->open(value_path, std::ios::in | std::ios::out);
    }

    void _unexport_gpio(const ChannelInfo& ch_info)
    {
        ch_info.f_direction->close();
        ch_info.f_value->close();
        string gpio_dir = _gpio_dir(ch_info);

        if (!os_path_exists(gpio_dir))
            return;

        ofstream f_unexport(_unexport_dir());
        f_unexport << ch_info.gpio;
    }

    void _output_one(const ChannelInfo& ch_info, const int value)
    {
        ch_info.f_value->seekg(0, std::ios::beg);
        *ch_info.f_value << static_cast<int>(static_cast<bool>(value));
        ch_info.f_value->flush();
    }

    void _setup_single_out(const ChannelInfo& ch_info, int initial = None)
    {
        _export_gpio(ch_info);

        ch_info.f_direction->seekg(0, std::ios::beg);
        *ch_info.f_direction << "out";
        ch_info.f_direction->flush();

        if (!is_None(initial))
            _output_one(ch_info, initial);

        _channel_configuration[ch_info.channel] = OUT;
    }

    void _setup_single_in(const ChannelInfo& ch_info)
    {
        _export_gpio(ch_info);

        ch_info.f_direction->seekg(0, std::ios::beg);
        *ch_info.f_direction << "in";
        ch_info.f_direction->flush();

        _channel_configuration[ch_info.channel] = IN;
    }

    string _pwm_path(const ChannelInfo& ch_info)
    {
        return format("%s/pwm%i", ch_info.pwm_chip_dir.c_str(), ch_info.pwm_id);
    }

    string _pwm_export_path(const ChannelInfo& ch_info) { return format("%s/export", ch_info.pwm_chip_dir.c_str()); }

    string _pwm_unexport_path(const ChannelInfo& ch_info)
    {
        return format("%s/unexport", ch_info.pwm_chip_dir.c_str());
    }

    string _pwm_period_path(const ChannelInfo& ch_info)
    {
        auto pwm_path = _pwm_path(ch_info);
        return format("%s/period", pwm_path.c_str());
    }

    string _pwm_duty_cycle_path(const ChannelInfo& ch_info)
    {
        auto pwm_path = _pwm_path(ch_info);
        return format("%s/duty_cycle", pwm_path.c_str());
    }

    string _pwm_enable_path(const ChannelInfo& ch_info)
    {
        auto pwm_path = _pwm_path(ch_info);
        return format("%s/enable", pwm_path.c_str());
    }

    void _export_pwm(const ChannelInfo& ch_info)
    {
        if (!os_path_exists(_pwm_path(ch_info)))
        { // scope for f
            string path = _pwm_export_path(ch_info);
            ofstream f(path);

            if (!f.is_open())
                throw runtime_error("Can't open " + path);

            f << ch_info.pwm_id;
        } // scope ends

        string enable_path = _pwm_enable_path(ch_info);

        int time_count = 0;
        while (!os_access(enable_path, R_OK | W_OK))
        {
            this_thread::sleep_for(chrono::milliseconds(10));
            if (time_count++ > 100)
                throw runtime_error("Permission denied: path: " + enable_path +
                                    "\n Please configure permissions or use the root user to run this.");
        }

        ch_info.f_duty_cycle->open(_pwm_duty_cycle_path(ch_info), std::ios::in | std::ios::out);
    }

    void _unexport_pwm(const ChannelInfo& ch_info)
    {
        ch_info.f_duty_cycle->close();

        ofstream f(_pwm_unexport_path(ch_info));
        f << ch_info.pwm_id;
    }

    void _set_pwm_period(const ChannelInfo& ch_info, const int period_ns)
    {
        ofstream f(_pwm_period_path(ch_info));
        f << period_ns;
    }

    void _set_pwm_duty_cycle(const ChannelInfo& ch_info, const int duty_cycle_ns)
    {
        // On boot, both period and duty cycle are both 0. In this state, the period
        // must be set first; any configuration change made while period==0 is
        // rejected. This is fine if we actually want a duty cycle of 0. Later, once
        // any period has been set, we will always be able to set a duty cycle of 0.
        // The code could be written to always read the current value, and only
        // write the value if the desired value is different. However, we enable
        // this check only for the 0 duty cycle case, to avoid having to read the
        // current value every time the duty cycle is set.
        if (duty_cycle_ns == 0)
        {
            ch_info.f_duty_cycle->seekg(0, std::ios::beg);

            auto cur = read(*ch_info.f_duty_cycle);
            cur = strip(cur);

            if (cur == "0")
                return;
        }

        ch_info.f_duty_cycle->seekg(0, std::ios::beg);
        *ch_info.f_duty_cycle << duty_cycle_ns;
        ch_info.f_duty_cycle->flush();
    }

    void _enable_pwm(const ChannelInfo& ch_info)
    {
        ofstream f(_pwm_enable_path(ch_info));
        f << 1;
    }

    void _disable_pwm(const ChannelInfo& ch_info)
    {
        ofstream f(_pwm_enable_path(ch_info));
        f << 0;
    }

    void _cleanup_one(const ChannelInfo& ch_info)
    {
        Directions app_cfg = _channel_configuration[ch_info.channel];
        if (app_cfg == HARD_PWM)
        {
            _disable_pwm(ch_info);
            _unexport_pwm(ch_info);
        }
        else
        {
            _event_cleanup(ch_info.gpio, ch_info.gpio_name);
            _unexport_gpio(ch_info);
        }
        _channel_configuration.erase(ch_info.channel);
    }

    void _cleanup_one(const std::string& channel)
    {
        ChannelInfo ch_info = _channel_to_info(channel);
        _cleanup_one(ch_info);
    }

    void _cleanup_all()
    {
        auto copied = _channel_configuration;
        for (const auto& _pair : copied)
        {
            const auto& channel = _pair.first;
            _cleanup_one(channel);
        }
        _gpio_mode = NumberingModes::None;
    }

    const std::string& model() const { return _model; }

    const std::string& JETSON_INFO() const { return _JETSON_INFO; }

private:
    MainModule()
    : _pinData(get_data()), // Get GPIO pin data
      _model(model_name(_pinData.model)),
      _JETSON_INFO(_pinData.pin_info.JETSON_INFO()),
      _channel_data_by_mode(_pinData.channel_data),
      _gpio_warnings(true),
      _gpio_mode(NumberingModes::None)
    {
        _check_permission();
    }

    void _check_permission() const
    {
        if (!os_access(_export_dir(), W_OK) || !os_access(_unexport_dir(), W_OK))
        {
            cerr << "[ERROR] The current user does not have permissions set to access the library functionalites. "
                    "Please configure permissions or use the root user to run this."
                 << endl;
            throw runtime_error("Permission Denied.");
        }
    }
};

//================================================================================

//================================================================================
// alias
MainModule& global() { return MainModule::get_instance(); }

//==================================================================================
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

//=======================================
// PWM

struct GPIO::PWM::Impl
{
    ChannelInfo _ch_info;
    bool _started = false;
    int _frequency_hz = 0;
    int _period_ns = 0;
    double _duty_cycle_percent = 0;
    int _duty_cycle_ns = 0;

    Impl(const std::string& channel, int frequency_hz) : _ch_info(global()._channel_to_info(channel, false, true))
    {
        try
        {
            Directions app_cfg = global()._app_channel_configuration(_ch_info);
            if (app_cfg == HARD_PWM)
                throw runtime_error("Can't create duplicate PWM objects");
            /*
            Apps typically set up channels as GPIO before making them be PWM,
            because RPi.GPIO does soft-PWM. We must undo the GPIO export to
            allow HW PWM to run on the pin.
            */
            if (app_cfg == IN || app_cfg == OUT)
                cleanup(channel);

            if (global()._gpio_warnings)
            {
                auto sysfs_cfg = global()._sysfs_channel_configuration(_ch_info);
                app_cfg = global()._app_channel_configuration(_ch_info);

                // warn if channel has been setup external to current program
                if (app_cfg == UNKNOWN && sysfs_cfg != UNKNOWN)
                {
                    cerr << "[WARNING] This channel is already in use, continuing "
                            "anyway. "
                            "Use GPIO::setwarnings(false) to disable warnings. "
                         << "channel: " << channel << endl;
                }
            }

            global()._export_pwm(_ch_info);
            global()._set_pwm_duty_cycle(_ch_info, 0);
            // Anything that doesn't match new frequency_hz
            _frequency_hz = -1 * frequency_hz;
            _reconfigure(frequency_hz, 0.0);
            global()._channel_configuration[channel] = HARD_PWM;
        }
        catch (exception& e)
        {
            throw _error(e, "GPIO::PWM::PWM()");
        }
    }

    Impl(int channel, int frequency_hz) : Impl(to_string(channel), frequency_hz) {}

    ~Impl()
    {
        if (!is_in(_ch_info.channel, global()._channel_configuration) ||
            global()._channel_configuration.at(_ch_info.channel) != HARD_PWM)
        {
            /* The user probably ran cleanup() on the channel already, so avoid
            attempts to repeat the cleanup operations. */
            return;
        }

        try
        {
            stop();
            global()._unexport_pwm(_ch_info);
            global()._channel_configuration.erase(_ch_info.channel);
        }
        catch (exception& e)
        {
            global()._cleanup_all();
            cerr << _error_message(e, "GPIO::PWM::~PWM()");
            terminate();
        }
        catch (...)
        {
            cerr << "[Exception] unknown error from GPIO::PWM::~PWM()! shut down the program." << endl;
            global()._cleanup_all();
            terminate();
        }
    }

    void start(double duty_cycle_percent)
    {
        try
        {
            _reconfigure(_frequency_hz, duty_cycle_percent, true);
        }
        catch (exception& e)
        {
            throw _error(e, "GPIO::PWM::start()");
        }
    }

    void ChangeFrequency(int frequency_hz)
    {
        try
        {
            _reconfigure(frequency_hz, _duty_cycle_percent);
        }
        catch (exception& e)
        {
            throw _error(e, "GPIO::PWM::ChangeFrequency()");
        }
    }

    void ChangeDutyCycle(double duty_cycle_percent)
    {
        try
        {
            _reconfigure(_frequency_hz, duty_cycle_percent);
        }
        catch (exception& e)
        {
            throw _error(e, "GPIO::PWM::ChangeDutyCycle()");
        }
    }

    void stop()
    {
        try
        {
            if (!_started)
                return;

            global()._disable_pwm(_ch_info);
        }
        catch (exception& e)
        {
            throw _error(e, "GPIO::PWM::stop()");
        }
    }

    void _reconfigure(int frequency_hz, double duty_cycle_percent, bool start = false)
    {
        if (duty_cycle_percent < 0.0 || duty_cycle_percent > 100.0)
            throw runtime_error("invalid duty_cycle_percent");

        bool freq_change = start || (frequency_hz != _frequency_hz);
        bool stop = _started && freq_change;

        if (stop)
        {
            _started = false;
            global()._disable_pwm(_ch_info);
        }

        if (freq_change)
        {
            _frequency_hz = frequency_hz;
            _period_ns = int(1000000000.0 / frequency_hz);
            // Reset duty cycle period incase the previous duty
            // cycle is higher than the period
            global()._set_pwm_duty_cycle(_ch_info, 0);
            global()._set_pwm_period(_ch_info, _period_ns);
        }

        bool duty_cycle_change = _duty_cycle_percent != duty_cycle_percent;
        if (duty_cycle_change || start || stop)
        {
            _duty_cycle_percent = duty_cycle_percent;
            _duty_cycle_ns = int(_period_ns * (duty_cycle_percent / 100.0));
            global()._set_pwm_duty_cycle(_ch_info, _duty_cycle_ns);
        }

        if (stop || start)
        {
            global()._enable_pwm(_ch_info);
            _started = true;
        }
    }
};

GPIO::PWM::PWM(const std::string& channel, int frequency_hz) : pImpl(make_unique<Impl>(channel, frequency_hz)) {}
GPIO::PWM::PWM(int channel, int frequency_hz) : pImpl(make_unique<Impl>(channel, frequency_hz)) {}
GPIO::PWM::~PWM() = default;

// move construct & assign
GPIO::PWM::PWM(GPIO::PWM&& other) = default;
GPIO::PWM& GPIO::PWM::operator=(GPIO::PWM&& other) = default;

void GPIO::PWM::start(double duty_cycle_percent) { pImpl->start(duty_cycle_percent); }

void GPIO::PWM::ChangeFrequency(int frequency_hz) { pImpl->ChangeFrequency(frequency_hz); }

void GPIO::PWM::ChangeDutyCycle(double duty_cycle_percent) { pImpl->ChangeDutyCycle(duty_cycle_percent); }

void GPIO::PWM::stop() { pImpl->stop(); }

//=======================================