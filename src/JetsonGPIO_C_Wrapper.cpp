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

#include "JetsonGPIO_C_Wrapper.h"
#include "JetsonGPIO.h"
#include <iostream>

#ifdef __cplusplus
extern "C"
{
#endif

    // check if enum size of c wrapper and the cpp code has the same length at compile time
    __attribute__((unused)) void check_enum_equality()
    {
#define CHECK_ENUM_EQUALITY_MSG "c wrapper enum must be equal to c++ enum"

        static_assert(static_cast<GPIONumberingModes>(GPIO::details::enum_size<GPIO::NumberingModes>::value) ==
                          GPIO_NUMBERING_MODES_SIZE,
                      CHECK_ENUM_EQUALITY_MSG);

        static_assert(static_cast<GPIODirections>(GPIO::details::enum_size<GPIO::Directions>::value) ==
                          GPIO_DIRECTIONS_SIZE,
                      CHECK_ENUM_EQUALITY_MSG);

        static_assert(static_cast<GPIOEdge>(GPIO::details::enum_size<GPIO::Edge>::value) == GPIO_EDGE_SIZE,
                      CHECK_ENUM_EQUALITY_MSG);

#undef CHECK_ENUM_EQUALITY_MSG
    }

    void gpio_setwarnings(bool state) { GPIO::setwarnings(state); }

    int gpio_setmode(GPIONumberingModes mode)
    {
        try
        {
            GPIO::setmode(static_cast<GPIO::NumberingModes>(mode));
            return 0;
        }
        catch (std::exception& e)
        {
            std::cerr << e.what();
            return -1;
        }
    }

    GPIONumberingModes gpio_getmode() { return static_cast<GPIONumberingModes>(GPIO::getmode()); }

    int gpio_setup(const char* channel, GPIODirections direction, int initial)
    {
        try
        {
            GPIO::setup(channel, static_cast<GPIO::Directions>(direction), initial);
            return 0;
        }
        catch (std::exception& e)
        {
            std::cerr << e.what();
            return -1;
        }
    }

    int gpio_cleanup(const char* channel)
    {
        try
        {
            GPIO::cleanup(channel);
            return 0;
        }
        catch (std::exception& e)
        {
            std::cerr << e.what();
            return -1;
        }
    }

    int gpio_input(const char* channel)
    {
        try
        {
            return GPIO::input(channel);
        }
        catch (std::exception& e)
        {
            std::cerr << e.what();
            return -1;
        }
    }

    int gpio_output(const char* channel, int value)
    {
        try
        {
            GPIO::output(channel, value);
            return 0;
        }
        catch (std::exception& e)
        {
            std::cerr << e.what();
            return -1;
        }
    }

    int gpio_gpio_function(const char* channel)
    {
        try
        {
            return static_cast<int>(GPIO::gpio_function(channel));
        }
        catch (std::exception& e)
        {
            std::cerr << e.what();
            return -1;
        }
    }

    int gpio_event_detected(const char* channel)
    {
        try
        {
            return static_cast<int>(GPIO::event_detected(channel));
        }
        catch (std::exception& e)
        {
            std::cerr << e.what();
            return -1;
        }
    }

    int gpio_add_event_callback(const char* channel, void (*callback)())
    {
        try
        {
            GPIO::add_event_callback(channel, callback);
            return 0;
        }
        catch (std::exception& e)
        {
            std::cerr << e.what();
            return -1;
        }
    }

    int gpio_remove_event_callback(const char* channel, void (*callback)())
    {
        try
        {
            GPIO::remove_event_callback(channel, callback);
            return 0;
        }
        catch (std::exception& e)
        {
            std::cerr << e.what();
            return -1;
        }
    }

    int gpio_add_event_detect(const char* channel, GPIOEdge edge, void (*callback)(), unsigned long bounce_time)
    {
        try
        {
            GPIO::add_event_detect(channel, static_cast<GPIO::Edge>(edge), callback, bounce_time);
            return 0;
        }
        catch (std::exception& e)
        {
            std::cerr << e.what();
            return -1;
        }
    }

    void gpio_remove_event_detect(const char* channel) { GPIO::remove_event_detect(channel); }

    int gpio_wait_for_edge(const char* channel, GPIOEdge edge, unsigned long bounce_time, unsigned long timeout)
    {
        try
        {
            auto wait_result = GPIO::wait_for_edge(channel, static_cast<GPIO::Edge>(edge), bounce_time, timeout);
            if (wait_result.is_event_detected())
            {
                return std::stoi(wait_result.channel());
            }
            else
            {
                return 0;
            }
        }
        catch (std::exception& e)
        {
            std::cerr << e.what();
            return -1;
        }
    }

#ifdef __cplusplus
}
#endif