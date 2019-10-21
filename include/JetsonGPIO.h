/*
Copyright (c) 2012-2017 Ben Croston <ben@croston.org>.
Copyright (c) 2019, NVIDIA CORPORATION. 
Copyright (c) 2019 Jueon Park <bluegbg@gmail.com>.

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
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.

==========================================================================
This is a simple C++ Jetson GPIO Library written by Jueon Park(pjueon). 
It has been written in C++ based on NVIDIA Jetson-GPIO Python Library, 
but it DOESN'T support all functionalites of the NVIDIA's original one. 

And it's never been tested yet.
==========================================================================
*/

#pragma once
#ifndef JETSON_GPIO_H
#define JETSON_GPIO_H

#include <memory> // for pImpl
#include <string>


namespace GPIO{
    constexpr auto VERSION = "0.0.1";
    extern const std::string JETSON_INFO;
    extern const std::string model;


    // Pin Numbering Modes
    // The user can use GPIO::BOARD, GPIO::BCM, GPIO::TEGRA_SOC, GPIO::CVM
    enum class NumberingModes;
    
    extern const NumberingModes BOARD;
    extern const NumberingModes BCM;
    extern const NumberingModes TEGRA_SOC;
    extern const NumberingModes CVM;

   // Pull up/down options are removed because they are unused in NVIDIA's original python libarary.
   // check: https://github.com/NVIDIA/jetson-gpio/issues/5

    constexpr int HIGH = 1;
    constexpr int LOW = 0;

    // GPIO directions. The user can use GPIO::IN, GPIO::OUT
    enum class Directions;
   
    extern const Directions IN;
    extern const Directions OUT;
    
    // Function used to enable/disable warnings during setup and cleanup.
    void setwarnings(bool state);

    // Function used to set the pin mumbering mode. 
    // Possible mode values are BOARD, BCM, TEGRA_SOC and CVM
    void setmode(NumberingModes mode);

    // Function used to get the currently set pin numbering mode
    NumberingModes getmode();

    /* Function used to setup individual pins as Input or Output. 
       direction must be IN or OUT, initial must be 
       HIGH or LOW and is only valid when direction is OUT  */
    void setup(const std::string& channel, Directions direction, int initial = -1);
    void setup(int channel, Directions direction, int initial = -1);


    /* Function used to cleanup channels at the end of the program.
       If no channel is provided, all channels are cleaned */
    void cleanup(const std::string& channel = "None");
    void cleanup(int channel);
    

    /* Function used to return the current value of the specified channel.
       Function returns either HIGH or LOW */
    int input(const std::string& channel);
    int input(int channel);
    

    /* Function used to set a value to a channel.
       Values must be either HIGH or LOW */
    void output(const std::string& channel, int value);
    void output(int channel, int value);

    /* Function used to check the currently set function of the channel specified. */
    Directions gpio_function(const std::string& channel);
    Directions gpio_function(int channel);

   //----------------------------------
    class PWM{
    public:
      PWM(int channel, int frequency_hz);
      PWM(const PWM&) = delete;              // Can't create duplicate PWM objects
      PWM& operator=(const PWM&) = delete;   // Can't create duplicate PWM objects
      ~PWM();
      void start(double duty_cycle_percent);
      void stop();
      void ChangeFrequency(int frequency_hz);
      void ChangeDutyCycle(double duty_cycle_percent);

   private:
      struct Impl;
      const std::unique_ptr<Impl> pImpl;
    };
}

#endif  // JETSON_GPIO_H
