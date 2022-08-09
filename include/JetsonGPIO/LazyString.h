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
#ifndef LAZY_STRING_H
#define LAZY_STRING_H

#include <functional>
#include <string>
#include <type_traits>

namespace GPIO
{
    class LazyString // lazily evaluated string object
    {
    public:
        LazyString(const std::function<std::string(void)>& func);
        LazyString(const std::string& str);
        LazyString(const char* str);

        LazyString(const LazyString&) = default;
        LazyString(LazyString&&) = default;

        LazyString& operator=(const LazyString&) = default;
        LazyString& operator=(LazyString&&) = default;

        operator const char*() const;
        operator std::string() const;
        const std::string& operator()() const;

    private:
        mutable std::string buffer;
        mutable bool is_cached;
        std::function<std::string(void)> func;
        void Evaluate() const;
    };

    namespace details
    {
        template <class T>
        constexpr bool is_string =
            std::is_constructible<LazyString, T>::value && !std::is_same<std::decay_t<T>, std::nullptr_t>::value;

        template <class T1, class T2>
        constexpr bool is_lazy_string_comparision = is_string<T1>&& is_string<T2> &&
                                                    (std::is_same<std::decay_t<T1>, LazyString>::value ||
                                                     std::is_same<std::decay_t<T2>, LazyString>::value);
    } // namespace details

} // namespace GPIO

template <class T1, class T2, class = std::enable_if_t<GPIO::details::is_lazy_string_comparision<T1, T2>>>
bool operator==(T1&& a, T2&& b)
{
    GPIO::LazyString _a(std::forward<T1>(a));
    GPIO::LazyString _b(std::forward<T2>(b));
    return _a() == _b();
}

template <class T1, class T2, class = std::enable_if_t<GPIO::details::is_lazy_string_comparision<T1, T2>>>
bool operator!=(T1&& a, T2&& b)
{
    return !(a == b);
}

#endif
