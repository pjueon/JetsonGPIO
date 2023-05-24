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
#ifndef MODEL_H
#define MODEL_H

#include <string>

namespace GPIO
{
    // enum
    enum class Model
    {
        CLARA_AGX_XAVIER,
        JETSON_NX,
        JETSON_XAVIER,
        JETSON_TX2,
        JETSON_TX1,
        JETSON_NANO,
        JETSON_TX2_NX,
        JETSON_ORIN,
        JETSON_ORIN_NX,
        JETSON_ORIN_NANO,
    };

    // names
    constexpr const char* MODEL_NAMES[] = {"CLARA_AGX_XAVIER", "JETSON_NX",   "JETSON_XAVIER", "JETSON_TX2",
                                           "JETSON_TX1",       "JETSON_NANO", "JETSON_TX2_NX", "JETSON_ORIN", "JETSON_ORIN_NX", "JETSON_ORIN_NANO"};

    // alias
    constexpr Model CLARA_AGX_XAVIER = Model::CLARA_AGX_XAVIER;
    constexpr Model JETSON_NX = Model::JETSON_NX;
    constexpr Model JETSON_XAVIER = Model::JETSON_XAVIER;
    constexpr Model JETSON_TX2 = Model::JETSON_TX2;
    constexpr Model JETSON_TX1 = Model::JETSON_TX1;
    constexpr Model JETSON_NANO = Model::JETSON_NANO;
    constexpr Model JETSON_TX2_NX = Model::JETSON_TX2_NX;
    constexpr Model JETSON_ORIN = Model::JETSON_ORIN;
    constexpr Model JETSON_ORIN_NX = Model::JETSON_ORIN_NX;
    constexpr Model JETSON_ORIN_NANO = Model::JETSON_ORIN_NANO;
} // namespace GPIO

#endif
