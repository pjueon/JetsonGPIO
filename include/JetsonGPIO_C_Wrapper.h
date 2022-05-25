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

#ifndef JETSON_GPIO_C_WRAPPER_H
#define JETSON_GPIO_C_WRAPPER_H

#include "stdbool.h"

#ifdef __cplusplus
extern "C"
{
#endif
    enum GPIONumberingModes
    {
        GPIO_NUMBERING_MODES_BOARD,
        GPIO_NUMBERING_MODES_BCM,
        GPIO_NUMBERING_MODES_TEGRA_SOC,
        GPIO_NUMBERING_MODES_CVM,
        GPIO_NUMBERING_MODES_None,
        GPIO_NUMBERING_MODES_SIZE // has to be in here for checking if enum is changed in c header, needs to be last
                                  // element
    };

    enum GPIODirections
    {
        GPIO_DIRECTIONS_UNKNOWN,
        GPIO_DIRECTIONS_OUT,
        GPIO_DIRECTIONS_IN,
        GPIO_DIRECTIONS_HARD_PWM,
        GPIO_DIRECTIONS_SIZE // has to be in here for checking if enum is changed in c header, needs to be last element
    };
    enum GPIOEdge
    {
        GPIO_EDGE_UNKNOWN,
        GPIO_EDGE_NONE,
        GPIO_EDGE_RISING,
        GPIO_EDGE_FALLING,
        GPIO_EDGE_BOTH,
        GPIO_EDGE_SIZE // has to be in here for checking if enum is changed in c header, needs to be last element
    };

    // Function used to enable/disable warnings during setup and cleanup.
    void gpio_setwarnings(bool state);

    // Function used to set the pin numbering mode.
    // Possible mode values are BOARD, BCM, TEGRA_SOC and CVM
    int gpio_setmode(enum GPIONumberingModes mode);

    // Function used to get the currently set pin numbering mode
    enum GPIONumberingModes gpio_getmode();

    /* Function used to setup individual pins as Input or Output.
       direction must be IN or OUT, initial must be
       HIGH or LOW and is only valid when direction is OUT
       @returns 0 on success -1 on failure
       */
    int gpio_setup(const char* channel, enum GPIODirections direction, int initial);

    /* Function used to cleanup channels at the end of the program.
       If no channel is provided, all channels are cleaned
       returns 0 on success -1 on failure*/
    int gpio_cleanup(const char* channel);

    /* Function used to return the current value of the specified channel.
       Function returns either HIGH or LOW or -1 on failure*/
    int gpio_input(const char* channel);

    /* Function used to set a value to a channel.
       Values must be either HIGH or LOW
       returns 0 on success -1 on failure*/
    int gpio_output(const char* channel, int value);

    /* Function used to check the currently set function of the channel specified.
       returns the function of the channel according to Directions
       returns -1 on failure*/
    int gpio_gpio_function(const char* channel);

    /* Function used to check if an event occurred on the specified channel.
       Param channel must be an integer.
       This function return True or False
       returns -1 on failure*/
    int gpio_event_detected(const char* channel);

    /* Function used to add a callback function to channel, after it has been
       registered for events using add_event_detect()
       returns 0 on success -1 on failure*/
    int gpio_add_event_callback(const char* channel, void (*callback)());

    /* Function used to remove a callback function previously added to detect a channel event
     * returns 0 on success -1 on failure*/
    int gpio_remove_event_callback(const char* channel, void (*callback)());

    /* Function used to add threaded event detection for a specified gpio channel.
       @gpio must be an integer specifying the channel
       @edge must be a member of Edge
       @callback (optional) may be a callback function to be called when the event is detected (or nullptr)
       @bouncetime (optional) a button-bounce signal ignore time (in milliseconds, default=none)
       @return 0 on success -1 on failure*/
    int gpio_add_event_detect(const char* channel, enum GPIOEdge edge, void (*callback)(), unsigned long bounce_time);

    /* Function used to remove event detection for channel */
    void gpio_remove_event_detect(const char* channel);

    /* Function used to perform a blocking wait until the specified edge event is detected within the specified
       timeout period. Returns the channel if an event is detected or 0 if a timeout has occurred.
       @channel is an integer specifying the channel
       @edge must be a member of GPIOEdge
       @bouncetime in milliseconds (optional)
       @timeout in milliseconds (optional)
       @returns channel number if detected, 0 on timeout, -1 on failure*/
    int gpio_wait_for_edge(const char* channel, enum GPIOEdge edge, unsigned long bounce_time, unsigned long timeout);

#ifdef __cplusplus
}
#endif

#endif // JETSON_GPIO_C_WRAPPER_H