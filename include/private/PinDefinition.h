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
#ifndef PIN_DEFINITION_H
#define PIN_DEFINITION_H

#include "JetsonGPIO.h"
#include "private/PythonFunctions.h"

#include <string>

struct PinDefinition
{
    const std::string LinuxPin;    // Linux GPIO pin number
    const std::string SysfsDir;    // GPIO chip sysfs directory
    const std::string BoardPin;    // Pin number (BOARD mode)
    const std::string BCMPin;      // Pin number (BCM mode)
    const std::string CVMPin;      // Pin name (CVM mode)
    const std::string TEGRAPin;    // Pin name (TEGRA_SOC mode)
    const std::string PWMSysfsDir; // PWM chip sysfs directory
    const int PWMID;               // PWM ID within PWM chip

    int LinuxPinNum () const
    {
        if(is_None(LinuxPin))
            return -1;

        try
        {
            return std::stoi(strip(LinuxPin)); 
        }
        catch(std::exception&)
        {
            return -1;
        }
    }

    std::string PinName(GPIO::NumberingModes key) const
    {
        if (key == GPIO::BOARD)
            return BoardPin;
        else if (key == GPIO::BCM)
            return BCMPin;
        else if (key == GPIO::CVM)
            return CVMPin;
        else // TEGRA_SOC
            return TEGRAPin;
    }
};

#endif
