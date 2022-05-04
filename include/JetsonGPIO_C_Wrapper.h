/*
Copyright (c) 2012-2017 Ben Croston ben@croston.org.
Copyright (c) 2019, NVIDIA CORPORATION.
Copyright (c) 2019 Jueon Park(pjueon) bluegbg@gmail.com.
Copyright (c) 2021 Adam Rasburn blackforestcheesecake@protonmail.ch
Copyright (c) 2022 Matthias Ludwig (ma-ludw)

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

#ifndef JETSON_GPIO_C_WRAPPER_H
#define JETSON_GPIO_C_WRAPPER_H

#include "JetsonGPIO.h"

#ifdef __cplusplus
extern "C" {
#endif

    // Function used to enable/disable warnings during setup and cleanup.
    void setwarnings(bool state);

    // Function used to set the pin mumbering mode.
    // Possible mode values are BOARD, BCM, TEGRA_SOC and CVM
    int setmode(GPIO::NumberingModes mode);

    // Function used to get the currently set pin numbering mode
    GPIO::NumberingModes getmode();

    /* Function used to setup individual pins as Input or Output.
       direction must be IN or OUT, initial must be
       HIGH or LOW and is only valid when direction is OUT
       @returns 0 on success -1 on failure
       */
    int setup(int channel, GPIO::Directions direction, int initial = -1);

    /* Function used to cleanup channels at the end of the program.
       If no channel is provided, all channels are cleaned
       returns 0 on success -1 on failure*/
    int cleanup(int channel);

    /* Function used to return the current value of the specified channel.
       Function returns either HIGH or LOW or -1 on failure*/
    int input(int channel);

    /* Function used to set a value to a channel.
       Values must be either HIGH or LOW
       returns 0 on success -1 on failure*/
    int output(int channel, int value);

    /* Function used to check the currently set function of the channel specified.
       returns the function of the channel according to Directions
       returns -1 on failure*/
    int gpio_function(int channel);


    /* Function used to check if an event occurred on the specified channel.
       Param channel must be an integer.
       This function return True or False
       returns -1 on failure*/
    int event_detected(int channel);

    /* Function used to add a callback function to channel, after it has been
       registered for events using add_event_detect()
       returns 0 on success -1 on failure*/
    int add_event_callback(int channel, void (*callback)());

    /* Function used to remove a callback function previously added to detect a channel event
     * returns 0 on success -1 on failure*/
    int remove_event_callback(int channel, void (*callback)());

    /* Function used to add threaded event detection for a specified gpio channel.
       @gpio must be an integer specifying the channel
       @edge must be a member of GPIO::Edge
       @callback (optional) may be a callback function to be called when the event is detected (or nullptr)
       @bouncetime (optional) a button-bounce signal ignore time (in milliseconds, default=none)
       @return 0 on success -1 on failure*/
    int add_event_detect(int channel, GPIO::Edge edge, void (*callback)(), unsigned long bounce_time = 0);

    /* Function used to remove event detection for channel */
    void remove_event_detect(int channel);

    /* Function used to perform a blocking wait until the specified edge event is detected within the specified
       timeout period. Returns the channel if an event is detected or 0 if a timeout has occurred.
       @channel is an integer specifying the channel
       @edge must be a member of GPIO::Edge
       @bouncetime in milliseconds (optional)
       @timeout in milliseconds (optional)
       @returns channel number if detected, 0 on timeout, -1 on failure*/
    int wait_for_edge(int channel, GPIO::Edge edge, unsigned long bounce_time = 0, unsigned long timeout = 0);


#ifdef __cplusplus
}
#endif

#endif // JETSON_GPIO_C_WRAPPER_H