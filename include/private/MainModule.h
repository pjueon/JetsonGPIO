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
#ifndef MAIN_MODULE_H
#define MAIN_MODULE_H

#include "private/GPIOPinData.h"
#include "private/PythonFunctions.h"

namespace GPIO
{
    class MainModule
    {
        // NOTE: DON'T change the declaration order of fields.
        // declaration order == initialization order
    private:
        PinData _pinData;
        std::string _model;
        std::string _JETSON_INFO;
        std::string _gpio_dir(const ChannelInfo& ch_info);

    public:
        const std::map<GPIO::NumberingModes, std::map<std::string, ChannelInfo>> _channel_data_by_mode;

        // A map used as lookup tables for pin to linux gpio mapping
        std::map<std::string, ChannelInfo> _channel_data;

        bool _gpio_warnings;
        NumberingModes _gpio_mode;
        std::map<std::string, Directions> _channel_configuration;

        MainModule(const MainModule&) = delete;
        MainModule& operator=(const MainModule&) = delete;

        ~MainModule();
        static MainModule& get_instance();

        void _validate_mode_set();
        ChannelInfo _channel_to_info_lookup(const std::string& channel, bool need_gpio, bool need_pwm);
        ChannelInfo _channel_to_info(const std::string& channel, bool need_gpio = false, bool need_pwm = false);

        std::vector<ChannelInfo> _channels_to_infos(const std::vector<std::string>& channels, bool need_gpio = false,
                                                    bool need_pwm = false);

        /* Return the current configuration of a channel as reported by sysfs.
           Any of IN, OUT, HARD_PWM, or UNKNOWN may be returned. */
        Directions _sysfs_channel_configuration(const ChannelInfo& ch_info);

        /* Return the current configuration of a channel as requested by this
           module in this process. Any of IN, OUT, or UNKNOWN may be returned. */
        Directions _app_channel_configuration(const ChannelInfo& ch_info);

        void _export_gpio(const ChannelInfo& ch_info);

        void _unexport_gpio(const ChannelInfo& ch_info);

        void _output_one(const ChannelInfo& ch_info, const int value);

        void _setup_single_out(const ChannelInfo& ch_info, int initial = None);

        void _setup_single_in(const ChannelInfo& ch_info);

        std::string _pwm_path(const ChannelInfo& ch_info);

        std::string _pwm_export_path(const ChannelInfo& ch_info);

        std::string _pwm_unexport_path(const ChannelInfo& ch_info);

        std::string _pwm_period_path(const ChannelInfo& ch_info);

        std::string _pwm_duty_cycle_path(const ChannelInfo& ch_info);

        std::string _pwm_enable_path(const ChannelInfo& ch_info);

        void _export_pwm(const ChannelInfo& ch_info);

        void _unexport_pwm(const ChannelInfo& ch_info);

        void _set_pwm_period(const ChannelInfo& ch_info, const int period_ns);

        void _set_pwm_duty_cycle(const ChannelInfo& ch_info, const int duty_cycle_ns);

        void _enable_pwm(const ChannelInfo& ch_info);

        void _disable_pwm(const ChannelInfo& ch_info);

        void _cleanup_one(const ChannelInfo& ch_info);

        void _cleanup_one(const std::string& channel);

        void _cleanup_all();

        void _warn_if_no_channel_to_cleanup();

        const std::string& model() const;

        const std::string& JETSON_INFO() const;

    private:
        MainModule();
        void _check_permission() const;
    };

    // alias (only for implementation)
    constexpr Directions UNKNOWN = Directions::UNKNOWN;
    constexpr Directions HARD_PWM = Directions::HARD_PWM;
    MainModule& global();

} // namespace GPIO
#endif
