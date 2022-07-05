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

#include <iostream>
#include <thread>
#include <unistd.h>

#include "JetsonGPIO.h"
#include "private/ExceptionHandling.h"
#include "private/GPIOEvent.h"
#include "private/MainModule.h"
#include "private/ModelUtility.h"
#include "private/SysfsRoot.h"

using namespace std;

namespace GPIO
{
    MainModule& global() { return MainModule::get_instance(); }

    // All global variables are wrapped in a singleton class except for public APIs,
    // in order to avoid initialization order problem among global variables in
    // different compilation units.
    string MainModule::_gpio_dir(const ChannelInfo& ch_info)
    {
        return format("%s/%s", _SYSFS_ROOT, ch_info.gpio_name.c_str());
    }

    MainModule::~MainModule()
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

    MainModule& MainModule::get_instance()
    {
        static MainModule singleton{};
        return singleton;
    }

    void MainModule::_validate_mode_set()
    {
        if (_gpio_mode == NumberingModes::None)
            throw runtime_error(
                "Please set pin numbering mode using "
                "GPIO::setmode(GPIO::BOARD), GPIO::setmode(GPIO::BCM), GPIO::setmode(GPIO::TEGRA_SOC) or "
                "GPIO::setmode(GPIO::CVM)");
    }

    ChannelInfo MainModule::_channel_to_info_lookup(const string& channel, bool need_gpio, bool need_pwm)
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

    ChannelInfo MainModule::_channel_to_info(const string& channel, bool need_gpio, bool need_pwm)
    {
        _validate_mode_set();
        return _channel_to_info_lookup(channel, need_gpio, need_pwm);
    }

    vector<ChannelInfo> MainModule::_channels_to_infos(const vector<string>& channels, bool need_gpio, bool need_pwm)
    {
        _validate_mode_set();
        vector<ChannelInfo> ch_infos{};
        for (const auto& c : channels)
        {
            ch_infos.push_back(_channel_to_info_lookup(c, need_gpio, need_pwm));
        }
        return ch_infos;
    }

    Directions MainModule::_sysfs_channel_configuration(const ChannelInfo& ch_info)
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

    Directions MainModule::_app_channel_configuration(const ChannelInfo& ch_info)
    {
        if (!is_in(ch_info.channel, _channel_configuration))
            return UNKNOWN; // Originally returns None in NVIDIA's GPIO Python Library
        return _channel_configuration[ch_info.channel];
    }

    void MainModule::_export_gpio(const ChannelInfo& ch_info)
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

    void MainModule::_unexport_gpio(const ChannelInfo& ch_info)
    {
        ch_info.f_direction->close();
        ch_info.f_value->close();
        string gpio_dir = _gpio_dir(ch_info);

        if (!os_path_exists(gpio_dir))
            return;

        ofstream f_unexport(_unexport_dir());
        f_unexport << ch_info.gpio;
    }

    void MainModule::_output_one(const ChannelInfo& ch_info, const int value)
    {
        ch_info.f_value->seekg(0, std::ios::beg);
        *ch_info.f_value << static_cast<int>(static_cast<bool>(value));
        ch_info.f_value->flush();
    }

    void MainModule::_setup_single_out(const ChannelInfo& ch_info, int initial)
    {
        _export_gpio(ch_info);

        ch_info.f_direction->seekg(0, std::ios::beg);
        *ch_info.f_direction << "out";
        ch_info.f_direction->flush();

        if (!is_None(initial))
            _output_one(ch_info, initial);

        _channel_configuration[ch_info.channel] = OUT;
    }

    void MainModule::_setup_single_in(const ChannelInfo& ch_info)
    {
        _export_gpio(ch_info);

        ch_info.f_direction->seekg(0, std::ios::beg);
        *ch_info.f_direction << "in";
        ch_info.f_direction->flush();

        _channel_configuration[ch_info.channel] = IN;
    }

    string MainModule::_pwm_path(const ChannelInfo& ch_info)
    {
        return format("%s/pwm%i", ch_info.pwm_chip_dir.c_str(), ch_info.pwm_id);
    }

    string MainModule::_pwm_export_path(const ChannelInfo& ch_info)
    {
        return format("%s/export", ch_info.pwm_chip_dir.c_str());
    }

    string MainModule::_pwm_unexport_path(const ChannelInfo& ch_info)
    {
        return format("%s/unexport", ch_info.pwm_chip_dir.c_str());
    }

    string MainModule::_pwm_period_path(const ChannelInfo& ch_info)
    {
        auto pwm_path = _pwm_path(ch_info);
        return format("%s/period", pwm_path.c_str());
    }

    string MainModule::_pwm_duty_cycle_path(const ChannelInfo& ch_info)
    {
        auto pwm_path = _pwm_path(ch_info);
        return format("%s/duty_cycle", pwm_path.c_str());
    }

    string MainModule::_pwm_enable_path(const ChannelInfo& ch_info)
    {
        auto pwm_path = _pwm_path(ch_info);
        return format("%s/enable", pwm_path.c_str());
    }

    void MainModule::_export_pwm(const ChannelInfo& ch_info)
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

    void MainModule::_unexport_pwm(const ChannelInfo& ch_info)
    {
        ch_info.f_duty_cycle->close();

        ofstream f(_pwm_unexport_path(ch_info));
        f << ch_info.pwm_id;
    }

    void MainModule::_set_pwm_period(const ChannelInfo& ch_info, const int period_ns)
    {
        ofstream f(_pwm_period_path(ch_info));
        f << period_ns;
    }

    void MainModule::_set_pwm_duty_cycle(const ChannelInfo& ch_info, const int duty_cycle_ns)
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

    void MainModule::_enable_pwm(const ChannelInfo& ch_info)
    {
        ofstream f(_pwm_enable_path(ch_info));
        f << 1;
    }

    void MainModule::_disable_pwm(const ChannelInfo& ch_info)
    {
        ofstream f(_pwm_enable_path(ch_info));
        f << 0;
    }

    void MainModule::_cleanup_one(const ChannelInfo& ch_info)
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

    void MainModule::_cleanup_one(const std::string& channel)
    {
        ChannelInfo ch_info = _channel_to_info(channel);
        _cleanup_one(ch_info);
    }

    void MainModule::_cleanup_all()
    {
        auto copied = _channel_configuration;
        for (const auto& _pair : copied)
        {
            const auto& channel = _pair.first;
            _cleanup_one(channel);
        }
        _gpio_mode = NumberingModes::None;
    }

    const std::string& MainModule::model() const { return _model; }

    const std::string& MainModule::JETSON_INFO() const { return _JETSON_INFO; }

    void MainModule::_warn_if_no_channel_to_cleanup()
    {
        // warn if no channel is setup
        if (global()._gpio_mode == NumberingModes::None && global()._gpio_warnings)
        {
            std::cerr << "[WARNING] No channels have been set up yet - nothing to clean up! "
                         "Try cleaning up at the end of your program instead!"
                      << std::endl;
            return;
        }
    }

    MainModule::MainModule()
    : _pinData(get_data()), // Get GPIO pin data
      _model(model_name(_pinData.model)),
      _JETSON_INFO(_pinData.pin_info.JETSON_INFO()),
      _channel_data_by_mode(_pinData.channel_data),
      _gpio_warnings(true),
      _gpio_mode(NumberingModes::None)
    {
        _check_permission();
    }

    void MainModule::_check_permission() const
    {
        if (!os_access(_export_dir(), W_OK) || !os_access(_unexport_dir(), W_OK))
        {
            cerr << "[ERROR] The current user does not have permissions set to access the library functionalites. "
                    "Please configure permissions or use the root user to run this."
                 << endl;
            throw runtime_error("Permission Denied.");
        }
    }

} // namespace GPIO
