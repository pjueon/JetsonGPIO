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
#include "private/Model.h"


struct PinInfo
{
    const int P1_REVISION;
    const std::string RAM;
    const std::string REVISION;
    const std::string TYPE;
    const std::string MANUFACTURER;
    const std::string PROCESSOR;
};

struct ChannelInfo
{
    const std::string channel;
    const std::string gpio_chip_dir;
    // const int chip_gpio;
    const int gpio;
    const std::string gpio_name;
    const std::string pwm_chip_dir;
    const int pwm_id;

    ChannelInfo(const std::string &channel, const std::string &gpio_chip_dir,
                // int chip_gpio, 
                int gpio, const std::string& gpio_name, const std::string &pwm_chip_dir,
                int pwm_id)
        : channel(channel),
          gpio_chip_dir(gpio_chip_dir),
        //   chip_gpio(chip_gpio),
          gpio(gpio),
          gpio_name(gpio_name),
          pwm_chip_dir(pwm_chip_dir),
          pwm_id(pwm_id)
    {}
};

struct PinData
{
    Model model;
    PinInfo pin_info;
    std::map<GPIO::NumberingModes, std::map<std::string, ChannelInfo>> channel_data;
};

PinData get_data();

#endif // GPIO_PIN_DATA_H
