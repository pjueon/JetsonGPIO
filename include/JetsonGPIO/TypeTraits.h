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
#ifndef TYPE_TRAITS_H
#define TYPE_TRAITS_H

#include <type_traits>

#ifndef CPP14_SUPPORTED
// define C++14 features
namespace std
{
    template <class T> using decay_t = typename decay<T>::type;
    template <bool B, class T = void> using enable_if_t = typename enable_if<B, T>::type;
} // namespace std
#endif

#ifndef CPP17_SUPPORTED
// define C++17 features
namespace std
{
    template <class...> using void_t = void;
}
#endif

namespace GPIO
{
    template <class T, class = void> struct is_equality_comparable : std::false_type
    {
    };

    template <class T>
    struct is_equality_comparable<T, std::void_t<decltype(std::declval<T>() == std::declval<T>())>>
    : std::is_convertible<decltype(std::declval<T>() == std::declval<T>()), bool>
    {
    };

    template <class T> constexpr bool is_equality_comparable_v = is_equality_comparable<T>::value;
} // namespace GPIO

#endif
