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

#include <functional>
#include <memory> // for pImpl
#include <string>
#include <type_traits>

#include "JetsonGPIOConfig.h"

#if (__cplusplus >= 201402L) && !defined(CPP14_SUPPORTED)
#define CPP14_SUPPORTED
#endif

#if (__cplusplus >= 201703L) && !defined(CPP17_SUPPORTED)
#define CPP17_SUPPORTED
#endif

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
    constexpr auto VERSION = JETSONGPIO_VERSION;

    extern const std::string JETSON_INFO;
    extern const std::string model;

    // Pin Numbering Modes
    enum class NumberingModes
    {
        BOARD,
        BCM,
        TEGRA_SOC,
        CVM,
        None
    };

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
    enum class Directions
    {
        UNKNOWN,
        OUT,
        IN,
        HARD_PWM
    };

    // GPIO::IN, GPIO::OUT
    constexpr Directions IN = Directions::IN;
    constexpr Directions OUT = Directions::OUT;

    // GPIO Event Types
    enum class Edge
    {
        UNKNOWN,
        NONE,
        RISING,
        FALLING,
        BOTH
    };

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
    void setup(const std::string& channel, Directions direction, int initial = -1);
    void setup(int channel, Directions direction, int initial = -1);

    /* Function used to cleanup channels at the end of the program.
       If no channel is provided, all channels are cleaned */
    void cleanup(const std::string& channel = "None");
    void cleanup(int channel);

    /* Function used to return the current value of the specified channel.
       Function returns either HIGH or LOW */
    int input(const std::string& channel);
    int input(int channel);

    /* Function used to set a value to a channel.
       Values must be either HIGH or LOW */
    void output(const std::string& channel, int value);
    void output(int channel, int value);

    /* Function used to check the currently set function of the channel specified. */
    Directions gpio_function(const std::string& channel);
    Directions gpio_function(int channel);

    //----------------------------------

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

        template <class T, class = void> struct is_equality_comparable : std::false_type
        {
        };

        template <class T>
        struct is_equality_comparable<T, std::void_t<decltype(std::declval<T>() == std::declval<T>())>>
        : std::is_convertible<decltype(std::declval<T>() == std::declval<T>()), bool>
        {
        };

        template <class T> constexpr bool is_equality_comparable_v = is_equality_comparable<T>::value;

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

            static_assert(is_equality_comparable_v<const T&>,
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

    class WaitResult
    {
    public:
        WaitResult(const std::string& channel);
        WaitResult(const WaitResult&) = default;
        WaitResult(WaitResult&&) = default;
        WaitResult& operator=(const WaitResult&) = default;
        WaitResult& operator=(WaitResult&&) = default;

        inline const std::string& channel() const { return _channel; }
        bool is_event_detected() const;
        inline operator bool() const { return is_event_detected(); }

    private:
        std::string _channel;
    };

    //----------------------------------

    /* Function used to check if an event occurred on the specified channel.
       Param channel must be an integer.
       This function return True or False */
    bool event_detected(const std::string& channel);
    bool event_detected(int channel);

    /* Function used to add a callback function to channel, after it has been
       registered for events using add_event_detect() */
    void add_event_callback(const std::string& channel, const Callback& callback);
    void add_event_callback(int channel, const Callback& callback);

    /* Function used to remove a callback function previously added to detect a channel event */
    void remove_event_callback(const std::string& channel, const Callback& callback);
    void remove_event_callback(int channel, const Callback& callback);

    /* Function used to add threaded event detection for a specified gpio channel.
       @gpio must be an integer specifying the channel
       @edge must be a member of GPIO::Edge
       @callback (optional) may be a callback function to be called when the event is detected (or nullptr)
       @bouncetime (optional) a button-bounce signal ignore time (in milliseconds, default=none) */
    void add_event_detect(const std::string& channel, Edge edge, const Callback& callback = nullptr,
                          unsigned long bounce_time = 0);
    void add_event_detect(int channel, Edge edge, const Callback& callback = nullptr, unsigned long bounce_time = 0);

    /* Function used to remove event detection for channel */
    void remove_event_detect(const std::string& channel);
    void remove_event_detect(int channel);

    /* Function used to perform a blocking wait until the specified edge event is detected within the specified
       timeout period. Returns the channel if an event is detected or 0 if a timeout has occurred.
       @channel is an integer specifying the channel
       @edge must be a member of GPIO::Edge
       @bouncetime in milliseconds (optional)
       @timeout in milliseconds (optional)
       @returns WaitResult object*/
    WaitResult wait_for_edge(const std::string& channel, Edge edge, unsigned long bounce_time = 0,
                             unsigned long timeout = 0);
    WaitResult wait_for_edge(int channel, Edge edge, unsigned long bounce_time = 0, unsigned long timeout = 0);

    //----------------------------------

    class PWM
    {
    public:
        PWM(int channel, int frequency_hz);
        PWM(PWM&& other);
        PWM& operator=(PWM&& other);
        PWM(const PWM&) = delete;            // Can't create duplicate PWM objects
        PWM& operator=(const PWM&) = delete; // Can't create duplicate PWM objects
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
