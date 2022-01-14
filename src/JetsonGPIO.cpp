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

#include <dirent.h>
#include <unistd.h>

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
#include <utility>
#include <vector>

#include "JetsonGPIO.h"

#include "private/Model.h"
#include "private/PythonFunctions.h"
#include "private/gpio_event.h"
#include "private/gpio_pin_data.h"
#include "private/ExceptionHandling.h"

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


class GlobalVariablesForGPIO
{
public:
    // -----Global Variables----
    // NOTE: DON'T change the declaration order of fields.
    // declaration order == initialization order

    PinData _pinData;
    const Model _model;
    const PinInfo _JETSON_INFO;
    const map<GPIO::NumberingModes, map<string, ChannelInfo>> _channel_data_by_mode;

    // A map used as lookup tables for pin to linux gpio mapping
    map<string, ChannelInfo> _channel_data;

    bool _gpio_warnings;
    NumberingModes _gpio_mode;
    map<string, Directions> _channel_configuration;

    GlobalVariablesForGPIO(const GlobalVariablesForGPIO&) = delete;
    GlobalVariablesForGPIO& operator=(const GlobalVariablesForGPIO&) = delete;

    static GlobalVariablesForGPIO& get_instance()
    {
        static GlobalVariablesForGPIO singleton{};
        return singleton;
    }

    std::string model_name() const
    {
        switch (_model) {
        case CLARA_AGX_XAVIER:
            return "CLARA_AGX_XAVIER";
        case JETSON_NX:
            return "JETSON_NX";
        case JETSON_XAVIER:
            return "JETSON_XAVIER";
        case JETSON_TX1:
            return "JETSON_TX1";
        case JETSON_TX2:
            return "JETSON_TX2";
        case JETSON_NANO:
            return "JETSON_NANO";
        case JETSON_TX2_NX:
            return "JETSON_TX2_NX";
        default:
            throw std::runtime_error("model_name error");
        }
    }

    string JETSON_INFO() const
    {
        stringstream ss{};
        ss << "[JETSON_INFO]\n";
        ss << "P1_REVISION: " << _JETSON_INFO.P1_REVISION << endl;
        ss << "RAM: " << _JETSON_INFO.RAM << endl;
        ss << "REVISION: " << _JETSON_INFO.REVISION << endl;
        ss << "TYPE: " << _JETSON_INFO.TYPE << endl;
        ss << "MANUFACTURER: " << _JETSON_INFO.MANUFACTURER << endl;
        ss << "PROCESSOR: " << _JETSON_INFO.PROCESSOR << endl;
        return ss.str();
    }


private:
    GlobalVariablesForGPIO()
    : _pinData(get_data()), // Get GPIO pin data
      _model(_pinData.model),
      _JETSON_INFO(_pinData.pin_info),
      _channel_data_by_mode(_pinData.channel_data),
      _gpio_warnings(true),
      _gpio_mode(NumberingModes::None)
    {
        _check_permission();
    }

    void _check_permission() const
    {
        string path1 = _SYSFS_ROOT + "/export"s;
        string path2 = _SYSFS_ROOT + "/unexport"s;
        if (!os_access(path1, W_OK) || !os_access(path2, W_OK)) {
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
GlobalVariablesForGPIO& global()
{
    return GlobalVariablesForGPIO::get_instance();
}


void _validate_mode_set()
{
    if (global()._gpio_mode == NumberingModes::None)
        throw runtime_error("Please set pin numbering mode using "
                            "GPIO::setmode(GPIO::BOARD), GPIO::setmode(GPIO::BCM), GPIO::setmode(GPIO::TEGRA_SOC) or "
                            "GPIO::setmode(GPIO::CVM)");
}

ChannelInfo _channel_to_info_lookup(const string& channel, bool need_gpio, bool need_pwm)
{
    if (!is_in(channel, global()._channel_data))
        throw runtime_error("Channel " + channel + " is invalid");
    ChannelInfo ch_info = global()._channel_data.at(channel);
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

vector<ChannelInfo> _channels_to_infos(const vector<string>& channels, bool need_gpio = false, bool need_pwm = false)
{
    _validate_mode_set();
    vector<ChannelInfo> ch_infos{};
    for (const auto& c : channels) {
        ch_infos.push_back(_channel_to_info_lookup(c, need_gpio, need_pwm));
    }
    return ch_infos;
}

/* Return the current configuration of a channel as reported by sysfs.
   Any of IN, OUT, HARD_PWM, or UNKNOWN may be returned. */
Directions _sysfs_channel_configuration(const ChannelInfo& ch_info)
{
    if (!is_None(ch_info.pwm_chip_dir)) {
        string pwm_dir = format("%s/pwm%i", ch_info.pwm_chip_dir.c_str(), ch_info.pwm_id);
        if (os_path_exists(pwm_dir))
            return HARD_PWM;
    }

    string gpio_dir = _SYSFS_ROOT + "/"s + ch_info.gpio_name;
    if (!os_path_exists(gpio_dir))
        return UNKNOWN; // Originally returns None in NVIDIA's GPIO Python Library

    string gpio_direction{};
    { // scope for f
        ifstream f_direction(gpio_dir + "/direction");
        stringstream buffer{};
        buffer << f_direction.rdbuf();
        gpio_direction = buffer.str();
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
    if (!is_in(ch_info.channel, global()._channel_configuration))
        return UNKNOWN; // Originally returns None in NVIDIA's GPIO Python Library
    return global()._channel_configuration[ch_info.channel];
}

void _export_gpio(const ChannelInfo& ch_info)
{
    if (!os_path_exists(_SYSFS_ROOT + "/"s + ch_info.gpio_name))
    { // scope for f_export
        ofstream f_export(_SYSFS_ROOT + "/export"s);
        f_export << ch_info.gpio;
    } // scope ends

    string value_path = _SYSFS_ROOT + "/"s + ch_info.gpio_name + "/value"s;

    int time_count = 0;
    while (!os_access(value_path, R_OK | W_OK)) {
        this_thread::sleep_for(chrono::milliseconds(10));
        if (time_count++ > 100)
            throw runtime_error("Permission denied: path: " + value_path +
                                "\n Please configure permissions or use the root user to run this.");                   
    }

    ch_info.f_direction->open(_SYSFS_ROOT + "/"s + ch_info.gpio_name + "/direction", std::ios::out);
    ch_info.f_value->open(value_path, std::ios::in | std::ios::out);
}

void _unexport_gpio(const ChannelInfo& ch_info)
{
    ch_info.f_direction->close();
    ch_info.f_value->close();

    if (!os_path_exists(_SYSFS_ROOT + "/"s + ch_info.gpio_name))
        return;

    ofstream f_unexport(_SYSFS_ROOT + "/unexport"s);
    f_unexport << ch_info.gpio;
}

void _output_one(const ChannelInfo& ch_info, const int value)
{
    ch_info.f_value->seekg(0, std::ios::beg);
    *ch_info.f_value << static_cast<int>(static_cast<bool>(value));
    ch_info.f_value->flush();
}

void _setup_single_out(const ChannelInfo& ch_info, int initial = -1)
{
    _export_gpio(ch_info);

    ch_info.f_direction->seekg(0, std::ios::beg);
    *ch_info.f_direction << "out";
    ch_info.f_direction->flush();

    if (initial != -1)
        _output_one(ch_info, initial);

    global()._channel_configuration[ch_info.channel] = OUT;
}

void _setup_single_in(const ChannelInfo& ch_info)
{
    _export_gpio(ch_info);

    ch_info.f_direction->seekg(0, std::ios::beg);
    *ch_info.f_direction << "in";
    ch_info.f_direction->flush();

    global()._channel_configuration[ch_info.channel] = IN;
}

string _pwm_path(const ChannelInfo& ch_info) { return ch_info.pwm_chip_dir + "/pwm" + to_string(ch_info.pwm_id); }

string _pwm_export_path(const ChannelInfo& ch_info) { return ch_info.pwm_chip_dir + "/export"; }

string _pwm_unexport_path(const ChannelInfo& ch_info) { return ch_info.pwm_chip_dir + "/unexport"; }

string _pwm_period_path(const ChannelInfo& ch_info) { return _pwm_path(ch_info) + "/period"; }

string _pwm_duty_cycle_path(const ChannelInfo& ch_info) { return _pwm_path(ch_info) + "/duty_cycle"; }

string _pwm_enable_path(const ChannelInfo& ch_info) { return _pwm_path(ch_info) + "/enable"; }

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
    while (!os_access(enable_path, R_OK | W_OK)) {
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
    if (duty_cycle_ns == 0) {
        ch_info.f_duty_cycle->seekg(0, std::ios::beg);
        stringstream buffer{};
        buffer << ch_info.f_duty_cycle->rdbuf();
        auto cur = buffer.str();
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
    Directions app_cfg = global()._channel_configuration[ch_info.channel];
    if (app_cfg == HARD_PWM) {
        _disable_pwm(ch_info);
        _unexport_pwm(ch_info);
    } else {
        _event_cleanup(ch_info.gpio, ch_info.gpio_name);
        _unexport_gpio(ch_info);
    }
    global()._channel_configuration.erase(ch_info.channel);
}

void _cleanup_all()
{
    auto copied = global()._channel_configuration;
    for (const auto& _pair : copied) {
        const auto& channel = _pair.first;
        ChannelInfo ch_info = _channel_to_info(channel);
        _cleanup_one(ch_info);
    }
    global()._gpio_mode = NumberingModes::None;
}




//==================================================================================
// APIs

// The reason that model and JETSON_INFO are not wrapped by
// GlobalVariablesForGPIO is because they belong to API


extern const string GPIO::model = global().model_name();
extern const string GPIO::JETSON_INFO = global().JETSON_INFO();

/* Function used to enable/disable warnings during setup and cleanup. */
void GPIO::setwarnings(bool state) { global()._gpio_warnings = state; }

// Function used to set the pin mumbering mode.
// Possible mode values are BOARD, BCM, TEGRA_SOC and CVM
void GPIO::setmode(NumberingModes mode)
{
    try {
        // check if mode is valid
        if (mode == NumberingModes::None)
            throw runtime_error("Pin numbering mode must be GPIO::BOARD, GPIO::BCM, GPIO::TEGRA_SOC or GPIO::CVM");
        // check if a different mode has been set
        if (global()._gpio_mode != NumberingModes::None && mode != global()._gpio_mode)
            throw runtime_error("A different mode has already been set!");

        global()._channel_data = global()._channel_data_by_mode.at(mode);
        global()._gpio_mode = mode;
    } catch (exception& e) {
        _cleanup_all();
        throw _error(e, "GPIO::setmode()");
    }
}

// Function used to get the currently set pin numbering mode
NumberingModes GPIO::getmode() { return global()._gpio_mode; }

/* Function used to setup individual pins or lists/tuples of pins as
   Input or Output. direction must be IN or OUT, initial must be
   HIGH or LOW and is only valid when direction is OUT  */
void GPIO::setup(const string& channel, Directions direction, int initial)
{
    try {
        ChannelInfo ch_info = _channel_to_info(channel, true);

        if (global()._gpio_warnings) {
            Directions sysfs_cfg = _sysfs_channel_configuration(ch_info);
            Directions app_cfg = _app_channel_configuration(ch_info);

            if (app_cfg == UNKNOWN && sysfs_cfg != UNKNOWN) {
                cerr << "[WARNING] This channel is already in use, continuing anyway. Use GPIO::setwarnings(false) to "
                        "disable warnings. channel: " << channel << endl;
            }
        }

        if (direction == OUT) {
            _setup_single_out(ch_info, initial);
        } else if (direction == IN) {
            if (initial != -1)
                throw runtime_error("initial parameter is not valid for inputs");
            _setup_single_in(ch_info);
        } else
            throw runtime_error("GPIO direction must be GPIO::IN or GPIO::OUT");
    } catch (exception& e) {
        _cleanup_all();
        throw _error(e, "GPIO::setup()");
    }
}

void GPIO::setup(int channel, Directions direction, int initial) { setup(to_string(channel), direction, initial); }

/* Function used to cleanup channels at the end of the program.
   If no channel is provided, all channels are cleaned */
void GPIO::cleanup(const string& channel)
{
    try {
        // warn if no channel is setup
        if (global()._gpio_mode == NumberingModes::None && global()._gpio_warnings) {
            cerr << "[WARNING] No channels have been set up yet - nothing to clean up! "
                    "Try cleaning up at the end of your program instead!" << endl;
            return;
        }

        // clean all channels if no channel param provided
        if (is_None(channel)) {
            _cleanup_all();
            return;
        }

        ChannelInfo ch_info = _channel_to_info(channel);
        if (is_in(ch_info.channel, global()._channel_configuration)) {
            _cleanup_one(ch_info);
        }
    } catch (exception& e) {
        _cleanup_all();
        throw _error(e, "GPIO::cleanup()");
    }
}

void GPIO::cleanup(int channel)
{
    string str_channel = to_string(channel);
    cleanup(str_channel);
}

/* Function used to return the current value of the specified channel.
   Function returns either HIGH or LOW */
int GPIO::input(const string& channel)
{
    try {
        ChannelInfo ch_info = _channel_to_info(channel, true);

        Directions app_cfg = _app_channel_configuration(ch_info);

        if (app_cfg != IN && app_cfg != OUT)
            throw runtime_error("You must setup() the GPIO channel first");

        ch_info.f_value->seekg(0, std::ios::beg);
        int value_read{};
        *ch_info.f_value >> value_read;
        return value_read;

    } catch (exception& e) {
        _cleanup_all();
        throw _error(e, "GPIO::input()");
    }
}

int GPIO::input(int channel) { return input(to_string(channel)); }

/* Function used to set a value to a channel.
   Values must be either HIGH or LOW */
void GPIO::output(const string& channel, int value)
{
    try {
        ChannelInfo ch_info = _channel_to_info(channel, true);
        // check that the channel has been set as output
        if (_app_channel_configuration(ch_info) != OUT)
            throw runtime_error("The GPIO channel has not been set up as an OUTPUT");
        _output_one(ch_info, value);
    } catch (exception& e) {
        _cleanup_all();
        throw _error(e, "GPIO::output()");
    }
}

void GPIO::output(int channel, int value) { output(to_string(channel), value); }

/* Function used to check the currently set function of the channel specified.
 */
Directions GPIO::gpio_function(const string& channel)
{
    try {
        ChannelInfo ch_info = _channel_to_info(channel);
        return _sysfs_channel_configuration(ch_info);
    } catch (exception& e) {
        _cleanup_all();
        throw _error(e, "GPIO::gpio_function()");
    }
}

Directions GPIO::gpio_function(int channel) { return gpio_function(to_string(channel)); }

//=============================== Events =================================

bool GPIO::event_detected(const std::string& channel)
{
    ChannelInfo ch_info = _channel_to_info(channel, true);
    try {
        // channel must be setup as input
        Directions app_cfg = _app_channel_configuration(ch_info);
        if (app_cfg != Directions::IN)
            throw runtime_error("You must setup() the GPIO channel as an input first");

        return _edge_event_detected(ch_info.gpio);
    } catch (exception& e) {
        _cleanup_all();
        throw _error(e, "GPIO::event_detected()");
    }
}

bool GPIO::event_detected(int channel) { return event_detected(std::to_string(channel)); }

void GPIO::add_event_callback(const std::string& channel, const Callback& callback)
{
    try {
        // Argument Check
        if (callback == nullptr) {
            throw invalid_argument("callback cannot be null");
        }

        ChannelInfo ch_info = _channel_to_info(channel, true);

        // channel must be setup as input
        Directions app_cfg = _app_channel_configuration(ch_info);
        if (app_cfg != Directions::IN) {
            throw runtime_error("You must setup() the GPIO channel as an input first");
        }

        // edge event must already exist
        if (_edge_event_exists(ch_info.gpio))
            throw runtime_error("The edge event must have been set via add_event_detect()");

        // Execute
        EventResultCode result = (EventResultCode)_add_edge_callback(ch_info.gpio, callback);
        switch (result) {
        case EventResultCode::None:
            break;
        default: {
            const char* error_msg = event_error_code_to_message[result];
            throw runtime_error(error_msg ? error_msg : "Unknown Error");
        }
        }
    } catch (exception& e) {
        _cleanup_all();
        throw _error(e, "GPIO::add_event_callback()");
    }
}

void GPIO::add_event_callback(int channel, const Callback& callback)
{
    add_event_callback(std::to_string(channel), callback);
}

void GPIO::remove_event_callback(const std::string& channel, const Callback& callback)
{
    ChannelInfo ch_info = _channel_to_info(channel, true);

    _remove_edge_callback(ch_info.gpio, callback);
}

void GPIO::remove_event_callback(int channel, const Callback& callback)
{
    remove_event_callback(std::to_string(channel), callback);
}

void GPIO::add_event_detect(const std::string& channel, Edge edge, const Callback& callback, unsigned long bounce_time)
{
    add_event_detect(std::atoi(channel.data()), edge, callback, bounce_time);
}

void GPIO::add_event_detect(int channel, Edge edge, const Callback& callback, unsigned long bounce_time)
{
    try {
        ChannelInfo ch_info = _channel_to_info(std::to_string(channel), true);

        // channel must be setup as input
        Directions app_cfg = _app_channel_configuration(ch_info);
        if (app_cfg != Directions::IN) {
            throw runtime_error("You must setup() the GPIO channel as an input first");
        }

        // edge provided must be rising, falling or both
        if (edge != Edge::RISING && edge != Edge::FALLING && edge != Edge::BOTH)
            throw invalid_argument("argument 'edge' must be set to RISING, FALLING or BOTH");

        // Execute
        EventResultCode result = (EventResultCode)_add_edge_detect(ch_info.gpio, ch_info.gpio_name, channel, edge, bounce_time);
        switch (result) {
        case EventResultCode::None:
            break;
        default: {
            const char* error_msg = event_error_code_to_message[result];
            throw runtime_error(error_msg ? error_msg : "Unknown Error");
        }
        }

        if (callback != nullptr) {
            if (_add_edge_callback(ch_info.gpio, callback))
                // Shouldn't happen (--it was just added successfully)
                throw runtime_error("Couldn't add callback due to unknown error with just added event");
        }
    } catch (exception& e) {
        _cleanup_all();
        throw _error(e, "GPIO::add_event_detect()");
    }
}

void GPIO::remove_event_detect(const std::string& channel)
{
    ChannelInfo ch_info = _channel_to_info(channel, true);

    _remove_edge_detect(ch_info.gpio, ch_info.gpio_name);
}

void GPIO::remove_event_detect(int channel) { remove_event_detect(std::to_string(channel)); }

int GPIO::wait_for_edge(const std::string& channel, Edge edge, uint64_t bounce_time, uint64_t timeout)
{
    return wait_for_edge(std::atoi(channel.data()), edge, bounce_time, timeout);
}

int GPIO::wait_for_edge(int channel, Edge edge, uint64_t bounce_time, uint64_t timeout)
{
    try {
        ChannelInfo ch_info = _channel_to_info(std::to_string(channel), true);

        // channel must be setup as input
        Directions app_cfg = _app_channel_configuration(ch_info);
        if (app_cfg != Directions::IN) {
            throw runtime_error("You must setup() the GPIO channel as an input first");
        }

        // edge provided must be rising, falling or both
        if (edge != Edge::RISING && edge != Edge::FALLING && edge != Edge::BOTH)
            throw invalid_argument("argument 'edge' must be set to RISING, FALLING or BOTH");

        // Execute
        EventResultCode result =
            (EventResultCode)_blocking_wait_for_edge(ch_info.gpio, ch_info.gpio_name, channel, edge, bounce_time, timeout);
        switch (result) {
        case EventResultCode::None:
            // Timeout
            return 0;
        case EventResultCode::EdgeDetected:
            // Event Detected
            return channel;
        default: {
            const char* error_msg = event_error_code_to_message[result];
            throw runtime_error(error_msg ? error_msg : "Unknown Error");
        }
        }
    } catch (exception& e) {
        _cleanup_all();
        throw _error(e, "GPIO::wait_for_edge()");
    }
}

//=======================================
// PWM 

struct GPIO::PWM::Impl {
    ChannelInfo _ch_info;
    bool _started = false;
    int _frequency_hz = 0;
    int _period_ns = 0;
    double _duty_cycle_percent = 0;
    int _duty_cycle_ns = 0;

    Impl(int channel, int frequency_hz) : _ch_info(_channel_to_info(to_string(channel), false, true))
    {
        try {
            Directions app_cfg = _app_channel_configuration(_ch_info);
            if (app_cfg == HARD_PWM)
                throw runtime_error("Can't create duplicate PWM objects");
            /*
            Apps typically set up channels as GPIO before making them be PWM,
            because RPi.GPIO does soft-PWM. We must undo the GPIO export to
            allow HW PWM to run on the pin.
            */
            if (app_cfg == IN || app_cfg == OUT) 
                cleanup(channel);
            

            if (global()._gpio_warnings) {
                auto sysfs_cfg = _sysfs_channel_configuration(_ch_info);
                app_cfg = _app_channel_configuration(_ch_info);

                // warn if channel has been setup external to current program
                if (app_cfg == UNKNOWN && sysfs_cfg != UNKNOWN) {
                    cerr << "[WARNING] This channel is already in use, continuing "
                            "anyway. "
                            "Use GPIO::setwarnings(false) to disable warnings. "
                         << "channel: " << channel << endl;
                }
            }

            _export_pwm(_ch_info);
            _set_pwm_duty_cycle(_ch_info, 0);
            // Anything that doesn't match new frequency_hz
            _frequency_hz = -1 * frequency_hz;
            _reconfigure(frequency_hz, 0.0);
            global()._channel_configuration[to_string(channel)] = HARD_PWM;
        } catch (exception& e) {
            _cleanup_all();
            throw _error(e, "GPIO::PWM::PWM()");
        }
    }

    ~Impl()
    {
        if (!is_in(_ch_info.channel, global()._channel_configuration) || 
            global()._channel_configuration.at(_ch_info.channel) != HARD_PWM) {
            /* The user probably ran cleanup() on the channel already, so avoid
            attempts to repeat the cleanup operations. */
            return;
        }

        try {
            stop();
            _unexport_pwm(_ch_info);
            global()._channel_configuration.erase(_ch_info.channel);
        } catch(exception& e) {
            _cleanup_all();
            cerr << _error_message(e, "GPIO::PWM::~PWM()");
            terminate();
        } 
        catch (...) {
            cerr << "[Exception] unknown error from GPIO::PWM::~PWM()! shut down the program." << endl;
            _cleanup_all();
            terminate();
        }
    }

    void start(double duty_cycle_percent)
    {
        try {
            _reconfigure(_frequency_hz, duty_cycle_percent, true);
        } catch (exception& e) {
            _cleanup_all();
            throw _error(e, "GPIO::PWM::start()");
        }
    }


    void ChangeFrequency(int frequency_hz)
    {
        try {
           _reconfigure(frequency_hz, _duty_cycle_percent);
        } catch (exception& e) {
            _cleanup_all();
            throw _error(e, "GPIO::PWM::ChangeFrequency()");
        }
    }

    void ChangeDutyCycle(double duty_cycle_percent)
    {
        try {
            _reconfigure(_frequency_hz, duty_cycle_percent);
        } catch (exception& e) {
            _cleanup_all();
            throw _error(e, "GPIO::PWM::ChangeDutyCycle()");
        }
    }


    void stop()
    {
        try {
            if (!_started)
                return;

            _disable_pwm(_ch_info);
        } catch (exception& e) {
            _cleanup_all();
            throw _error(e, "GPIO::PWM::stop()");
        }
    }


    void _reconfigure(int frequency_hz, double duty_cycle_percent, bool start = false)
    {
        if (duty_cycle_percent < 0.0 || duty_cycle_percent > 100.0)
            throw runtime_error("invalid duty_cycle_percent");

        bool freq_change = start || (frequency_hz != _frequency_hz);
        bool stop = _started && freq_change;

        if (stop) {
            _started = false;
            _disable_pwm(_ch_info);
        }

        if(freq_change)
        {
            _frequency_hz = frequency_hz;
            _period_ns = int(1000000000.0 / frequency_hz);
            _set_pwm_period(_ch_info, _period_ns);
        }

        _duty_cycle_percent = duty_cycle_percent;
        _duty_cycle_ns = int(_period_ns * (duty_cycle_percent / 100.0));
        _set_pwm_duty_cycle(_ch_info, _duty_cycle_ns);

        if (stop || start) {
            _enable_pwm(_ch_info);
            _started = true;
        }
    }
};


GPIO::PWM::PWM(int channel, int frequency_hz): pImpl(make_unique<Impl>(channel, frequency_hz)) {}
GPIO::PWM::~PWM() = default;

// move construct & assign
GPIO::PWM::PWM(GPIO::PWM&& other) = default;
GPIO::PWM& GPIO::PWM::operator=(GPIO::PWM&& other) = default;

void GPIO::PWM::start(double duty_cycle_percent)
{
    pImpl->start(duty_cycle_percent);
}

void GPIO::PWM::ChangeFrequency(int frequency_hz)
{
    pImpl->ChangeFrequency(frequency_hz);
}

void GPIO::PWM::ChangeDutyCycle(double duty_cycle_percent)
{
    pImpl->ChangeDutyCycle(duty_cycle_percent);
}

void GPIO::PWM::stop()
{
    pImpl->stop();
}

//=======================================


//=======================================
// Callback
void GPIO::Callback::operator()(int input) const
{
	if (function != nullptr)
		function(input);
}


bool GPIO::operator==(const GPIO::Callback& A, const GPIO::Callback& B)
{
	return A.comparer(A.function, B.function);
}


bool GPIO::operator!=(const GPIO::Callback& A, const GPIO::Callback& B)
{
	return !(A == B);
}
//=======================================


//=======================================
struct _cleaner {
private:
    _cleaner() = default;

public:
    _cleaner(const _cleaner&) = delete;
    _cleaner& operator=(const _cleaner&) = delete;

    static _cleaner& get_instance()
    {
        static _cleaner singleton;
        return singleton;
    }

    ~_cleaner()
    {
        try {
            // When the user forgot to call cleanup() at the end of the program,
            // _cleaner object will call it.
            _cleanup_all();
        } catch (exception& e) {
            cerr << _error_message(e, "~_cleaner()") << std::endl;
        }
    }
};

// AutoCleaner will be destructed at the end of the program, and call
// _cleanup_all(). It COULD cause a problem because at the end of the program,
// global()._channel_configuration and global()._gpio_mode MUST NOT be destructed
// before AutoCleaner. But the static objects are destructed in the reverse
// order of construction, and objects defined in the same compilation unit will
// be constructed in the order of definition. So it's supposed to work properly.
_cleaner& AutoCleaner = _cleaner::get_instance();

//=========================================================================
