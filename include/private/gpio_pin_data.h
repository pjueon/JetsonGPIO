/*
Copyright (c) 2012-2017 Ben Croston ben@croston.org.
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
#ifndef GPIO_PIN_DATA_H
#define GPIO_PIN_DATA_H

#include <string>
#include <map>
#include <vector>

#include "JetsonGPIO.h"


enum class Model;

struct _GPIO_PIN_DEF{
    const int LinuxPin;           // Linux GPIO pin number
    const std::string SysfsDir;        // GPIO chip sysfs directory
    const std::string BoardPin;        // Pin number (BOARD mode)
    const std::string BCMPin;          // Pin number (BCM mode)
    const std::string CVMPin;          // Pin name (CVM mode)
    const std::string TEGRAPin;        // Pin name (TEGRA_SOC mode)
    const std::string PWMSysfsDir;     // PWM chip sysfs directory
    const int PWMID;              // PWM ID within PWM chip
};  

struct _GPIO_PIN_INFO{
    const int P1_REVISION;
    const std::string RAM;
    const std::string REVISION;
    const std::string TYPE;
    const std::string MANUFACTURER;
    const std::string PROCESSOR;
};

extern const std::vector<_GPIO_PIN_DEF> JETSON_XAVIER_PIN_DEFS;
extern const std::vector<std::string> compats_xavier;

extern const std::vector<_GPIO_PIN_DEF> JETSON_TX2_PIN_DEFS;
extern const std::vector<std::string> compats_tx2;

extern const std::vector<_GPIO_PIN_DEF> JETSON_TX1_PIN_DEFS;
extern const std::vector<std::string> compats_tx1;

extern const std::vector<_GPIO_PIN_DEF> JETSON_NANO_PIN_DEFS;
extern const std::vector<std::string> compats_nano;

extern const std::map<Model, std::vector<_GPIO_PIN_DEF>> PIN_DEFS_MAP;
extern const std::map<Model, _GPIO_PIN_INFO> JETSON_INFO_MAP;


struct GPIO_data{
    Model model;
    _GPIO_PIN_INFO pin_info;
    map<NumberingModes, map<string, ChannelInfo>> channel_data;
};


struct ChannelInfo{
    ChannelInfo(const string& channel, const string& gpio_chip_dir, 
		int chip_gpio, int gpio, const string& pwm_chip_dir, 
		int pwm_id)
	    : channel(channel),
	      gpio_chip_dir(gpio_chip_dir),
	      chip_gpio(chip_gpio), 
	      gpio(gpio), 
	      pwm_chip_dir(pwm_chip_dir),
	      pwm_id(pwm_id) 
	    {}
    ChannelInfo(const ChannelInfo&) = default;

    const string channel;
    const string gpio_chip_dir;
    const int chip_gpio;
    const int gpio;
    const string pwm_chip_dir;
    const int pwm_id;
};


GPIO_data get_data();


#endif // GPIO_PIN_DATA_H