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

#include "private/Model.h"
#include <stdexcept>

namespace GPIO
{
    std::string model_name(Model model)
    {
        switch (model)
        {
        case CLARA_AGX_XAVIER:
            return "CLARA_AGX_XAVIER";
        case JETSON_NX:
            return "JETSON_NX";
        case JETSON_XAVIER:
            return "JETSON_XAVIER";
        case JETSON_TX1:
            return "JETSON_TX1";
        case JETSON_TX2:
            return "JETSON_TX2";
        case JETSON_NANO:
            return "JETSON_NANO";
        case JETSON_TX2_NX:
            return "JETSON_TX2_NX";
        case JETSON_ORIN:
            return "JETSON_ORIN";
        default:
            throw std::runtime_error("model_name error");
        }
    }
} // namespace GPIO