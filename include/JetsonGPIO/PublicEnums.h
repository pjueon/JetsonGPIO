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
#ifndef PUBLIC_ENUMS_H
#define PUBLIC_ENUMS_H

#include <cstdio> // for size_t

#define _ARG_COUNT(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, N, ...) N
#define _EXPAND(x) x
#define ARG_COUNT(...)                                                                                                 \
    _EXPAND(_ARG_COUNT(__VA_ARGS__, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1))

#define PUBLIC_ENUM_CLASS(NAME, ...)                                                                                   \
    enum class NAME                                                                                                    \
    {                                                                                                                  \
        __VA_ARGS__                                                                                                    \
    };                                                                                                                 \
                                                                                                                       \
    template <> struct details::enum_size<NAME>                                                                        \
    {                                                                                                                  \
        static constexpr size_t value = ARG_COUNT(__VA_ARGS__);                                                        \
    }

namespace GPIO
{
    namespace details
    {
        template <class E> struct enum_size
        {
        };
    } // namespace details

    // Pin Numbering Modes
    PUBLIC_ENUM_CLASS(NumberingModes, BOARD, BCM, TEGRA_SOC, CVM, None);

    // GPIO directions.
    // UNKNOWN constant is for gpios that are not yet setup
    // If the user uses UNKNOWN or HARD_PWM as a parameter to GPIO::setmode function,
    // An exception will occur
    PUBLIC_ENUM_CLASS(Directions, UNKNOWN, OUT, IN, HARD_PWM);

    // GPIO Event Types
    PUBLIC_ENUM_CLASS(Edge, UNKNOWN, NONE, RISING, FALLING, BOTH);

    // alias for GPIO::NumberingModes
    constexpr NumberingModes BOARD = NumberingModes::BOARD;
    constexpr NumberingModes BCM = NumberingModes::BCM;
    constexpr NumberingModes TEGRA_SOC = NumberingModes::TEGRA_SOC;
    constexpr NumberingModes CVM = NumberingModes::CVM;

    // Pull up/down options are removed because they are unused in NVIDIA's original python libarary.
    // check: https://github.com/NVIDIA/jetson-gpio/issues/5

    // alias for GPIO::Directions
    constexpr Directions IN = Directions::IN;
    constexpr Directions OUT = Directions::OUT;

    // alias for GPIO::Edge
    constexpr Edge NO_EDGE = Edge::NONE;
    constexpr Edge RISING = Edge::RISING;
    constexpr Edge FALLING = Edge::FALLING;
    constexpr Edge BOTH = Edge::BOTH;

} // namespace GPIO

#undef PUBLIC_ENUM_CLASS
#endif
