#pragma once

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
        if(LinuxPin == "None")
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