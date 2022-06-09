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

#include "JetsonGPIO.h"
#include "private/ExceptionHandling.h"
#include "private/MainModule.h"

namespace GPIO
{
    struct PWM::Impl
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
                    throw std::runtime_error("Can't create duplicate PWM objects");
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
                        std::cerr << "[WARNING] This channel is already in use, continuing "
                                     "anyway. "
                                     "Use setwarnings(false) to disable warnings. "
                                  << "channel: " << channel << std::endl;
                    }
                }

                global()._export_pwm(_ch_info);
                global()._set_pwm_duty_cycle(_ch_info, 0);
                // Anything that doesn't match new frequency_hz
                _frequency_hz = -1 * frequency_hz;
                _reconfigure(frequency_hz, 0.0);
                global()._channel_configuration[channel] = HARD_PWM;
            }
            catch (std::exception& e)
            {
                throw _error(e, "PWM::PWM()");
            }
        }

        Impl(int channel, int frequency_hz) : Impl(std::to_string(channel), frequency_hz) {}

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
            catch (std::exception& e)
            {
                global()._cleanup_all();
                std::cerr << _error_message(e, "PWM::~PWM()");
                std::terminate();
            }
            catch (...)
            {
                std::cerr << "[Exception] unknown error from PWM::~PWM()! shut down the program." << std::endl;
                global()._cleanup_all();
                std::terminate();
            }
        }

        void start(double duty_cycle_percent)
        {
            try
            {
                _reconfigure(_frequency_hz, duty_cycle_percent, true);
            }
            catch (std::exception& e)
            {
                throw _error(e, "PWM::start()");
            }
        }

        void ChangeFrequency(int frequency_hz)
        {
            try
            {
                _reconfigure(frequency_hz, _duty_cycle_percent);
            }
            catch (std::exception& e)
            {
                throw _error(e, "PWM::ChangeFrequency()");
            }
        }

        void ChangeDutyCycle(double duty_cycle_percent)
        {
            try
            {
                _reconfigure(_frequency_hz, duty_cycle_percent);
            }
            catch (std::exception& e)
            {
                throw _error(e, "PWM::ChangeDutyCycle()");
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
            catch (std::exception& e)
            {
                throw _error(e, "PWM::stop()");
            }
        }

        void _reconfigure(int frequency_hz, double duty_cycle_percent, bool start = false)
        {
            if (duty_cycle_percent < 0.0 || duty_cycle_percent > 100.0)
                throw std::runtime_error("invalid duty_cycle_percent");

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

    PWM::PWM(const std::string& channel, int frequency_hz) : pImpl(std::make_unique<Impl>(channel, frequency_hz)) {}
    PWM::PWM(int channel, int frequency_hz) : pImpl(std::make_unique<Impl>(channel, frequency_hz)) {}
    PWM::~PWM() = default;

    // move construct & assign
    PWM::PWM(PWM&& other) = default;
    PWM& PWM::operator=(PWM&& other) = default;

    void PWM::start(double duty_cycle_percent) { pImpl->start(duty_cycle_percent); }

    void PWM::ChangeFrequency(int frequency_hz) { pImpl->ChangeFrequency(frequency_hz); }

    void PWM::ChangeDutyCycle(double duty_cycle_percent) { pImpl->ChangeDutyCycle(duty_cycle_percent); }

    void PWM::stop() { pImpl->stop(); }

} // namespace GPIO
