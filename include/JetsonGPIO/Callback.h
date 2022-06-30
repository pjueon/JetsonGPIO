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
#ifndef CALLBACK_H
#define CALLBACK_H

#include "JetsonGPIO/TypeTraits.h"
#include <functional>
#include <string>

namespace GPIO
{
    // Callback definition
    class Callback;
    bool operator==(const Callback& A, const Callback& B);
    bool operator!=(const Callback& A, const Callback& B);

    namespace details
    {
        class NoArgCallback;

        // type traits
        enum class CallbackType
        {
            Normal,
            NoArg
        };

        template <CallbackType e> struct CallbackConstructorOverload
        {
        };

        template <class T>
        constexpr bool is_no_argument_callback =
            !std::is_same<std::decay_t<T>, NoArgCallback>::value && std::is_constructible<NoArgCallback, T&&>::value;

        template <class T>
        constexpr bool is_string_argument_callback =
            std::is_constructible<std::function<void(const std::string&)>, T&&>::value;

        template <class T> constexpr CallbackType CallbackTypeSelector()
        {
            static_assert(std::is_copy_constructible<std::decay_t<T>>::value, "Callback must be copy-constructible");

            static_assert(is_no_argument_callback<T> || is_string_argument_callback<T>, "Callback must be callable");

            static_assert(details::is_equality_comparable_v<const T&>,
                          "Callback function MUST be equality comparable. ex> f0 == f1");

            return is_string_argument_callback<T> ? CallbackType::Normal : CallbackType::NoArg;
        }

        template <class arg_t> struct CallbackCompare
        {
            using func_t = std::function<arg_t>;

            template <class T> static bool Compare(const func_t& A, const func_t& B)
            {
                if (A == nullptr && B == nullptr)
                    return true;

                const T* targetA = A.template target<T>();
                const T* targetB = B.template target<T>();

                return targetA != nullptr && targetB != nullptr && *targetA == *targetB;
            }
        };

        class NoArgCallback
        {
        private:
            using func_t = std::function<void()>;
            using comparer_impl = details::CallbackCompare<void()>;

        public:
            NoArgCallback(const NoArgCallback&) = default;
            NoArgCallback(NoArgCallback&&) = default;

            template <class T, class = std::enable_if_t<!std::is_same<std::decay_t<T>, nullptr_t>::value &&
                                                        std::is_constructible<func_t, T&&>::value>>
            NoArgCallback(T&& function)
            : function(std::forward<T>(function)), comparer(comparer_impl::Compare<std::decay_t<T>>)
            {
            }

            NoArgCallback& operator=(const NoArgCallback&) = default;
            NoArgCallback& operator=(NoArgCallback&&) = default;

            bool operator==(const NoArgCallback& other) const { return comparer(function, other.function); }
            bool operator!=(const NoArgCallback& other) const { return !(*this == other); }

            void operator()(const std::string&) const { function(); }

        private:
            func_t function;
            std::function<bool(const func_t&, const func_t&)> comparer;
        };

    } // namespace details

    class Callback
    {
    private:
        using func_t = std::function<void(const std::string&)>;
        using comparer_impl = details::CallbackCompare<void(const std::string&)>;

        template <class T>
        Callback(T&& function, details::CallbackConstructorOverload<details::CallbackType::Normal>)
        : function(std::forward<T>(function)), comparer(comparer_impl::Compare<std::decay_t<T>>)
        {
            static_assert(std::is_constructible<func_t, T&&>::value,
                          "Callback return type: void, argument type: const std::string&");
        }

        template <class T>
        Callback(T&& noArgFunction, details::CallbackConstructorOverload<details::CallbackType::NoArg>)
        : function(details::NoArgCallback(std::forward<T>(noArgFunction))),
          comparer(comparer_impl::Compare<details::NoArgCallback>)
        {
        }

    public:
        Callback(const Callback&) = default;
        Callback(Callback&&) = default;

        template <class T>
        Callback(T&& function)
        : Callback(std::forward<T>(function),
                   details::CallbackConstructorOverload<details::CallbackTypeSelector<T>()>{})
        {
        }

        Callback& operator=(const Callback&) = default;
        Callback& operator=(Callback&&) = default;

        void operator()(const std::string& input) const;

        friend bool operator==(const Callback& A, const Callback& B);
        friend bool operator!=(const Callback& A, const Callback& B);

    private:
        func_t function;
        std::function<bool(const func_t&, const func_t&)> comparer;
    };

} // namespace GPIO

#endif
