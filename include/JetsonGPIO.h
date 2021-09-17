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
#ifndef JETSON_GPIO_H
#define JETSON_GPIO_H

#include <memory> // for pImpl
#include <string>

namespace GPIO {
  constexpr auto VERSION = "0.1.3";
  constexpr auto _SYSFS_ROOT = "/sys/class/gpio";

  extern const std::string JETSON_INFO;
  extern const std::string model;

  // Pin Numbering Modes
  enum class NumberingModes { BOARD, BCM, TEGRA_SOC, CVM, None };

  // GPIO::BOARD, GPIO::BCM, GPIO::TEGRA_SOC, GPIO::CVM
  constexpr NumberingModes BOARD = NumberingModes::BOARD;
  constexpr NumberingModes BCM = NumberingModes::BCM;
  constexpr NumberingModes TEGRA_SOC = NumberingModes::TEGRA_SOC;
  constexpr NumberingModes CVM = NumberingModes::CVM;

  // Pull up/down options are removed because they are unused in NVIDIA's original python libarary.
  // check: https://github.com/NVIDIA/jetson-gpio/issues/5

  constexpr int HIGH = 1;
  constexpr int LOW = 0;

  // GPIO directions.
  // UNKNOWN constant is for gpios that are not yet setup
  // If the user uses UNKNOWN or HARD_PWM as a parameter to GPIO::setmode function,
  // An exception will occur
  enum class Directions { UNKNOWN, OUT, IN, HARD_PWM };

  // GPIO::IN, GPIO::OUT
  constexpr Directions IN = Directions::IN;
  constexpr Directions OUT = Directions::OUT;

  // GPIO Event Types
  enum class Edge { UNKNOWN, NONE, RISING, FALLING, BOTH };

  constexpr Edge NO_EDGE = Edge::NONE;
  constexpr Edge RISING = Edge::RISING;
  constexpr Edge FALLING = Edge::FALLING;
  constexpr Edge BOTH = Edge::BOTH;

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
  void setup(const std::string &channel, Directions direction, int initial = -1);
  void setup(int channel, Directions direction, int initial = -1);

  /* Function used to cleanup channels at the end of the program.
     If no channel is provided, all channels are cleaned */
  void cleanup(const std::string &channel = "None");
  void cleanup(int channel);

  /* Function used to return the current value of the specified channel.
     Function returns either HIGH or LOW */
  int input(const std::string &channel);
  int input(int channel);

  /* Function used to set a value to a channel.
     Values must be either HIGH or LOW */
  void output(const std::string &channel, int value);
  void output(int channel, int value);

  /* Function used to check the currently set function of the channel specified. */
  Directions gpio_function(const std::string &channel);
  Directions gpio_function(int channel);

  //----------------------------------

  /* Function used to check if an event occurred on the specified channel.
     Param channel must be an integer.
     This function return True or False */
  bool event_detected(const std::string &channel);
  bool event_detected(int channel);

  /* Function used to add a callback function to channel, after it has been
     registered for events using add_event_detect() */
  void add_event_callback(const std::string &channel, void (*callback)(int channel));
  void add_event_callback(int channel, void (*callback)(int channel));

  /* Function used to remove a callback function previously added to detect a channel event */
  void remove_event_callback(const std::string &channel, void (*callback)(int channel));
  void remove_event_callback(int channel, void (*callback)(int channel));

  /* Function used to add threaded event detection for a specified gpio channel.
     @gpio must be an integer specifying the channel
     @edge must be a member of GPIO::Edge
     @callback (optional) may be a callback function to be called when the event is detected (or nullptr)
     @bouncetime (optional) a button-bounce signal ignore time (in milliseconds, default=none) */
  void add_event_detect(const std::string &channel, Edge edge, void (*callback)(int channel) = nullptr,
                        unsigned long bounce_time = 0);
  void add_event_detect(int channel, Edge edge, void (*callback)(int channel) = nullptr, unsigned long bounce_time = 0);

  /* Function used to remove event detection for channel */
  void remove_event_detect(const std::string &channel);
  void remove_event_detect(int channel);

  /* Function used to perform a blocking wait until the specified edge event is detected within the specified
     timeout period. Returns the channel if an event is detected or 0 if a timeout has occurred.
     @channel is an integer specifying the channel
     @edge must be a member of GPIO::Edge
     @bouncetime in milliseconds (optional)
     @timeout in milliseconds (optional)
     @returns channel for an event, 0 for a timeout */
  int wait_for_edge(const std::string &channel, Edge edge, unsigned long bounce_time = 0, unsigned long timeout = 0);
  int wait_for_edge(int channel, Edge edge, unsigned long bounce_time = 0, unsigned long timeout = 0);

  //----------------------------------

  class PWM {
public:
    PWM(int channel, int frequency_hz);
    PWM(PWM &&other);
    PWM &operator=(PWM &&other);
    PWM(const PWM &) = delete;            // Can't create duplicate PWM objects
    PWM &operator=(const PWM &) = delete; // Can't create duplicate PWM objects
    ~PWM();
    void start(double duty_cycle_percent);
    void stop();
    void ChangeFrequency(int frequency_hz);
    void ChangeDutyCycle(double duty_cycle_percent);

private:
    struct Impl;
    std::unique_ptr<Impl> pImpl;
  };
} // namespace GPIO

#endif // JETSON_GPIO_H
