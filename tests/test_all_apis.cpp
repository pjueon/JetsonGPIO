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

#include <JetsonGPIO.h>
#include <algorithm>
#include <chrono>
#include <functional>
#include <future>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <tuple>
#include <utility>
#include <vector>

using namespace std::string_literals;

void sleep(double sec) { std::this_thread::sleep_for(std::chrono::duration<double>(sec)); }

template <class T, class Q> bool is_in(T&& element, Q&& container)
{
    return std::find(container.begin(), container.end(), element) != container.end();
}

// utility classes------
class Warning
{
public:
    Warning() : backup(std::cerr.rdbuf()), buf{} { std::cerr.rdbuf(buf.rdbuf()); }

    ~Warning() { std::cerr.rdbuf(backup); }

    Warning(const Warning&) = delete;

    std::string contents() const { return buf.str(); }

private:
    std::streambuf* backup;
    std::stringstream buf;
};

std::future<void> DelayedSetChannel(int channel, int value, double delay)
{
    auto run = [=]() 
    {
        sleep(delay);
        GPIO::output(channel, value);
    };

    return std::async(std::launch::async, run);
}

struct TestFunc
{
    std::function<void(void)> func;
    std::string name;
};

class TestCallback
{
public:
    TestCallback(bool* flag, const std::string& target) : flag(flag), target(target){};

    TestCallback(const TestCallback&) = default;

    bool operator==(const TestCallback& other) const { return other.flag == flag && other.target == target; }

    void operator()(const std::string& channel)
    {
        if (channel == target && flag != nullptr)
        {
            *flag = true;
        }
    }

private:
    bool* flag;
    std::string target;
};

// utility classes------

// If a board has PWM support, the PWM tests expect 'out_a' to be PWM-capable.
struct TestPinData
{
    // Board mode pins
    int out_a;
    int in_a;
    int out_b;
    int in_b;
    std::vector<int> unimplemented_pins;
    // Other pin modes
    std::string cvm_pin;
    std::string tegra_soc_pin;
    std::vector<int> all_pwms;
};

class APITests
{
private:
    static TestPinData get_test_pin_data(const std::string& model)
    {
        if (model == "JETSON_ORIN")
            /* Pre-test configuration, if boot-time pinmux doesn't set up PWM pins:
            Set BOARD pin 15 as mux function PWM:
            busybox devmem 0x02440020 32 0x400
            Set BOARD pin 18 as mux function PWM:
            busybox devmem 0x02434040 32 0x401
            Board mode pins */
            return {18, 19, 11, 13, {}, "GPIO40", "GP66", {15, 18}};

        if (model == "JETSON_XAVIER")
            /* Pre-test configuration, if boot-time pinmux doesn't set up PWM pins:
            Set BOARD pin 18 as mux function PWM:
            busybox devmem 0x2434090 32 0x401 */
            return {18, 19, 21, 22, {}, "MCLK05", "SOC_GPIO42", {13, 15, 18}};

        if (model == "JETSON_TX2")
            return {18, 19, 21, 22, {26}, "AUDIO_MCLK", "AUD_MCLK", {}};

        if (model == "JETSON_TX1")
            return {18, 19, 21, 22, {}, "AUDIO_MCLK", "AUD_MCLK", {}};

        if (model == "JETSON_NANO")
            /* Pre-test configuration, if boot-time pinmux doesn't set up PWM pins:
            Set BOARD pin 32 as mux function PWM (set bits 1:0 to 1 not 3):
            sudo busybox devmem 0x700031fc 32 0x45
            Set BOARD pin 32 as SFIO (clear bit 0):
            sudo busybox devmem 0x6000d504 32 0x2 */
            return {32, 31, 29, 26, {}, "GPIO9", "AUD_MCLK", {32, 33}};

        if (model == "JETSON_NX")
            /* Pre-test configuration, if boot-time pinmux doesn't set up PWM pins:
            Set BOARD pin 32 as mux function PWM (func 1):
            busybox devmem 0x2430040 32 0x401
            Set BOARD pin 33 as mux function PWM (func 2):
            busybox devmem 0x2440020 32 0x402 */
            return {32, 31, 29, 26, {}, "GPIO09", "AUD_MCLK", {32, 33}};

        if (model == "CLARA_AGX_XAVIER")
            /* Pre-test configuration, if boot-time pinmux doesn't set up PWM pins:
            Set BOARD pin 18 as mux function PWM:
            busybox devmem 0x2434090 32 0x401 */
            return {18, 19, 21, 22, {}, "MCLK05", "SOC_GPIO42", {15, 18}};

        if (model == "JETSON_TX2_NX")
            /* Pre-test configuration, if boot-time pinmux doesn't set up PWM pins:
            Set BOARD pin 33 as mux function PWM (func 1):
            busybox devmem 0x0c3010a8 32 0x401
            Set BOARD pin 32 as mux function PWM (func 2):
            busybox devmem 0x0c301080 32 0x401
            Board mode pins */
            return {32, 31, 29, 26, {}, "GPIO09", "AUD_MCLK", {32, 33}};

        throw std::runtime_error("invalid model");
    }

    TestPinData pin_data = get_test_pin_data(GPIO::model);
    std::vector<int> all_board_pins = {7,  11, 12, 13, 15, 16, 18, 19, 21, 22, 23,
                                       24, 26, 29, 31, 32, 33, 35, 36, 37, 38, 40};
    int bcm_pin = 4;
    std::vector<TestFunc> tests = {};

public:
    void run()
    {
        print_info();
        add_tests();

        for (auto& test : tests)
        {
            std::cout << "Testing " << test.name << std::endl;

            try
            {
                test.func();
            }
            catch (...)
            {
                std::cout << "test failed." << std::endl;
                GPIO::cleanup();
                throw;
            }
        }

        std::cout << "All tests passed." << std::endl;
    }

private:
    // test cases
    void test_warnings_off()
    {
        GPIO::setwarnings(false);
        Warning w{};

        // cleanup() warns if no GPIOs were set up
        GPIO::cleanup();

        if (!w.contents().empty())
            throw std::runtime_error("Unexpected warning occured");
    }

    void test_warnings_on()
    {
        GPIO::setwarnings(true);
        Warning w{};

        // cleanup() warns if no GPIOs were set up
        GPIO::cleanup();

        if (w.contents().empty())
            throw std::runtime_error("Expected warning did not occur");
    }

    void test_setup_one_board()
    {
        GPIO::setmode(GPIO::BOARD);
        assert(GPIO::getmode() == GPIO::BOARD);
        GPIO::setup(pin_data.in_a, GPIO::IN);
        GPIO::cleanup();
        assert(GPIO::getmode() == GPIO::NumberingModes::None);
    }

    void test_setup_one_bcm()
    {
        GPIO::setmode(GPIO::BCM);
        assert(GPIO::getmode() == GPIO::BCM);
        GPIO::setup(bcm_pin, GPIO::IN);
        GPIO::cleanup();
        assert(GPIO::getmode() == GPIO::NumberingModes::None);
    }

    void test_setup_one_cvm()
    {
        GPIO::setmode(GPIO::CVM);
        assert(GPIO::getmode() == GPIO::CVM);
        GPIO::setup(pin_data.cvm_pin, GPIO::IN);
        GPIO::cleanup();
        assert(GPIO::getmode() == GPIO::NumberingModes::None);
    }

    void test_setup_one_tegra_soc()
    {
        GPIO::setmode(GPIO::TEGRA_SOC);
        assert(GPIO::getmode() == GPIO::TEGRA_SOC);
        GPIO::setup(pin_data.tegra_soc_pin, GPIO::IN);
        GPIO::cleanup();
        assert(GPIO::getmode() == GPIO::NumberingModes::None);
    }

    void test_setup_one_out_no_init()
    {
        GPIO::setmode(GPIO::BOARD);
        GPIO::setup(pin_data.out_a, GPIO::OUT);
        GPIO::cleanup();
    }

    void test_setup_one_out_high()
    {
        GPIO::setmode(GPIO::BOARD);
        GPIO::setup(pin_data.out_a, GPIO::OUT, GPIO::HIGH);
        GPIO::cleanup();
    }

    void test_setup_one_out_low()
    {
        GPIO::setmode(GPIO::BOARD);
        GPIO::setup(pin_data.out_a, GPIO::OUT, GPIO::LOW);
        GPIO::cleanup();
    }

    void test_setup_one_in()
    {
        GPIO::setmode(GPIO::BOARD);
        GPIO::setup(pin_data.in_a, GPIO::IN);
        GPIO::cleanup();
    }

    void test_setup_all()
    {
        GPIO::setmode(GPIO::BOARD);
        for (auto pin : all_board_pins)
        {
            if (is_in(pin, pin_data.unimplemented_pins))
                continue;
            GPIO::setup(pin, GPIO::IN);
        }
        GPIO::cleanup();
    }

    void test_cleanup_one()
    {
        GPIO::setmode(GPIO::BOARD);
        GPIO::setup(pin_data.in_a, GPIO::IN);
        GPIO::cleanup(pin_data.in_a);
        assert(GPIO::getmode() == GPIO::BOARD);
        GPIO::cleanup();
        assert(GPIO::getmode() == GPIO::NumberingModes::None);
    }

    void test_cleanup_all()
    {
        GPIO::setmode(GPIO::BOARD);
        GPIO::setup(pin_data.in_a, GPIO::IN);
        GPIO::setup(pin_data.in_b, GPIO::IN);
        GPIO::cleanup();
        assert(GPIO::getmode() == GPIO::NumberingModes::None);
    }

    void test_input()
    {
        GPIO::setmode(GPIO::BOARD);
        GPIO::setup(pin_data.in_a, GPIO::IN);
        GPIO::input(pin_data.in_a);
        GPIO::cleanup();
    }

    void test_output_one()
    {
        GPIO::setmode(GPIO::BOARD);
        GPIO::setup(pin_data.out_a, GPIO::OUT);
        GPIO::output(pin_data.out_a, GPIO::HIGH);
        GPIO::output(pin_data.out_a, GPIO::LOW);
        GPIO::cleanup();
    }

    void test_out_in_init_high()
    {
        GPIO::setmode(GPIO::BOARD);
        GPIO::setup(pin_data.out_a, GPIO::OUT, GPIO::HIGH);
        GPIO::setup(pin_data.in_a, GPIO::IN);

        auto val = GPIO::input(pin_data.in_a);
        assert(val == GPIO::HIGH);

        GPIO::output(pin_data.out_a, GPIO::LOW);
        val = GPIO::input(pin_data.in_a);
        assert(val == GPIO::LOW);

        GPIO::output(pin_data.out_a, GPIO::HIGH);
        val = GPIO::input(pin_data.in_a);
        assert(val == GPIO::HIGH);

        GPIO::cleanup();
    }

    void test_out_in_init_low()
    {
        GPIO::setmode(GPIO::BOARD);
        GPIO::setup(pin_data.out_a, GPIO::OUT, GPIO::LOW);
        GPIO::setup(pin_data.in_a, GPIO::IN);

        auto val = GPIO::input(pin_data.in_a);
        assert(val == GPIO::LOW);

        GPIO::output(pin_data.out_a, GPIO::HIGH);
        val = GPIO::input(pin_data.in_a);
        assert(val == GPIO::HIGH);

        GPIO::output(pin_data.out_a, GPIO::LOW);
        val = GPIO::input(pin_data.in_a);
        assert(val == GPIO::LOW);

        GPIO::cleanup();
    }

    void test_gpio_function_unexported()
    {
        GPIO::setmode(GPIO::BOARD);
        auto val = GPIO::gpio_function(pin_data.in_a);
        assert(val == GPIO::Directions::UNKNOWN);
        GPIO::cleanup();
    }

    void test_gpio_function_in()
    {
        GPIO::setmode(GPIO::BOARD);
        GPIO::setup(pin_data.in_a, GPIO::IN);
        auto val = GPIO::gpio_function(pin_data.in_a);
        assert(val == GPIO::Directions::IN);
        GPIO::cleanup();
    }

    void test_gpio_function_out()
    {
        GPIO::setmode(GPIO::BOARD);
        GPIO::setup(pin_data.out_a, GPIO::OUT);
        auto val = GPIO::gpio_function(pin_data.out_a);
        assert(val == GPIO::Directions::OUT);
        GPIO::cleanup();
    }

    void test_wait_for_edge_rising()
    {
        GPIO::setmode(GPIO::BOARD);
        GPIO::setup(pin_data.out_a, GPIO::OUT, GPIO::LOW);
        GPIO::setup(pin_data.in_a, GPIO::IN);
        auto dsc = DelayedSetChannel(pin_data.out_a, GPIO::HIGH, 0.5);
        auto result = GPIO::wait_for_edge(pin_data.in_a, GPIO::RISING, 10, 1000);
        dsc.wait();
        assert(std::stoi(result.channel()) == pin_data.in_a);
        assert(result.is_event_detected());
        GPIO::cleanup();
    }

    void test_wait_for_edge_falling()
    {
        GPIO::setmode(GPIO::BOARD);
        GPIO::setup(pin_data.out_a, GPIO::OUT, GPIO::HIGH);
        GPIO::setup(pin_data.in_a, GPIO::IN);
        auto dsc = DelayedSetChannel(pin_data.out_a, GPIO::LOW, 0.5);
        auto result = GPIO::wait_for_edge(pin_data.in_a, GPIO::FALLING, 10, 1000);
        dsc.wait();
        assert(std::stoi(result.channel()) == pin_data.in_a);
        assert(result.is_event_detected());
        GPIO::cleanup();
    }

    // clang-format off

    void test_event_detected_rising()
    {
        _test_events(
            GPIO::HIGH, 
            GPIO::RISING, 
            { 
                {GPIO::LOW, false}, 
                {GPIO::HIGH, true}, 
                {GPIO::LOW, false}, 
                {GPIO::HIGH, true}
            }, 
            false, 
            false
        );
        _test_events(
            GPIO::LOW, 
            GPIO::RISING, 
            { 
                {GPIO::HIGH, true}, 
                {GPIO::LOW, false}, 
                {GPIO::HIGH, true},
                {GPIO::LOW, false} 
            }, 
            true, 
            false
        );
    }


    void test_event_detected_falling()
    {
        _test_events(
            GPIO::HIGH, 
            GPIO::FALLING, 
            { 
                {GPIO::LOW, true}, 
                {GPIO::HIGH, false}, 
                {GPIO::LOW, true}, 
                {GPIO::HIGH, false}
            }, 
            false, 
            false
        );
        _test_events(
            GPIO::LOW, 
            GPIO::FALLING, 
            { 
                {GPIO::HIGH, false}, 
                {GPIO::LOW, true}, 
                {GPIO::HIGH, false},
                {GPIO::LOW, true} 
            }, 
            true, 
            false
        );
    }


    void test_event_detected_both()
    {
        _test_events(
            GPIO::HIGH, 
            GPIO::BOTH, 
            { 
                {GPIO::LOW, true}, 
                {GPIO::HIGH, true}, 
                {GPIO::LOW, true}, 
                {GPIO::HIGH, true}
            }, 
            false, 
            false
        );
        _test_events(
            GPIO::LOW, 
            GPIO::BOTH, 
            { 
                {GPIO::HIGH, true}, 
                {GPIO::LOW, true}, 
                {GPIO::HIGH, true},
                {GPIO::LOW, true} 
            }, 
            false, 
            true
        );
    }

    // clang-format on

    // =========== pwm tests ===========

    void test_pwm_multi_duty()
    {
        for (auto pct : {25.0, 50.0, 75.0})
        {
            GPIO::setmode(GPIO::BOARD);
            GPIO::setup(pin_data.in_a, GPIO::IN);
            GPIO::setup(pin_data.out_a, GPIO::OUT, GPIO::HIGH);

            int count = 0;

            GPIO::PWM p(pin_data.out_a, 500);
            p.start(pct);
            constexpr int N = 5000;
            for (int i = 0; i < N; i++)
                count += GPIO::input(pin_data.in_a);

            p.stop();

            const auto delta = 5;
            const auto min_ct = N * (pct - delta) / 100.0;
            const auto max_ct = N * (pct + delta) / 100.0;

            assert(min_ct <= count && count <= max_ct);
            GPIO::cleanup();
        }
    }

    void test_pwm_change_frequency()
    {
        GPIO::setmode(GPIO::BOARD);
        GPIO::setup(pin_data.out_a, GPIO::OUT, GPIO::HIGH);
        GPIO::PWM p(pin_data.out_a, 500);
        p.start(50.0);
        p.ChangeFrequency(550);
        p.stop();
        GPIO::cleanup();
    }

    void test_pwm_change_duty_cycle()
    {
        GPIO::setmode(GPIO::BOARD);
        GPIO::setup(pin_data.out_a, GPIO::OUT, GPIO::HIGH);
        GPIO::PWM p(pin_data.out_a, 500);
        p.start(50.0);
        p.ChangeDutyCycle(60.0);
        p.stop();
        GPIO::cleanup();
    }

    void test_pwm_cleanup_none()
    {
        GPIO::setmode(GPIO::BOARD);
        GPIO::setup(pin_data.out_a, GPIO::OUT, GPIO::HIGH);
        GPIO::PWM p(pin_data.out_a, 500);
        p.start(50.0);
        GPIO::cleanup();
    }

    void test_pwm_cleanup_stop()
    {
        GPIO::setmode(GPIO::BOARD);
        GPIO::setup(pin_data.out_a, GPIO::OUT, GPIO::HIGH);
        GPIO::PWM p(pin_data.out_a, 500);
        p.start(50.0);
        p.stop();
        GPIO::cleanup();
    }

    void test_pwm_cleanup_del()
    {
        GPIO::setmode(GPIO::BOARD);
        GPIO::setup(pin_data.out_a, GPIO::OUT, GPIO::HIGH);
        { // scope for p
            GPIO::PWM p(pin_data.out_a, 500);
            p.start(50.0);
        }
        GPIO::cleanup();
    }

    void test_pwm_create_all()
    {
        for (auto& pin : pin_data.all_pwms)
        {
            GPIO::setmode(GPIO::BOARD);
            GPIO::setup(pin, GPIO::OUT, GPIO::HIGH);
            GPIO::PWM p(pin, 500);
            p.start(50.0);
            p.stop();
            GPIO::cleanup();
        }
    }

    void print_info()
    {
        const auto line = "==========================";

        std::cout << line << std::endl;
        std::cout << "[Library Version] " << GPIO::VERSION << std::endl;
        std::cout << "[Model] " << GPIO::model << std::endl;
        std::cout << GPIO::JETSON_INFO;
        std::cout << line << std::endl;

        std::cout << "[NOTE]" << std::endl;
        std::cout << "For this test script to run correctly, ";
        std::cout << "you must connect a wire between" << std::endl;
        std::cout << "the pin the test uses for output(BOARD pin ";
        std::cout << pin_data.out_a << ")";
        std::cout << " and the pin the test uses for input(BOARD pin " << pin_data.in_a << ").";
        std::cout << std::endl;

        std::cout << line << std::endl;
    }

    void add_tests()
    {
        tests.reserve(100);
#define ADD_TEST(NAME) tests.push_back({[this]() { NAME(); }, #NAME})

        ADD_TEST(test_warnings_off);
        ADD_TEST(test_warnings_on);
        ADD_TEST(test_setup_one_board);
        ADD_TEST(test_setup_one_bcm);
        ADD_TEST(test_setup_one_cvm);
        ADD_TEST(test_setup_one_tegra_soc);
        ADD_TEST(test_setup_one_out_no_init);
        ADD_TEST(test_setup_one_out_high);
        ADD_TEST(test_setup_one_out_low);
        ADD_TEST(test_setup_one_in);
        ADD_TEST(test_setup_all);
        ADD_TEST(test_cleanup_one);
        ADD_TEST(test_cleanup_all);
        ADD_TEST(test_input);
        ADD_TEST(test_output_one);
        ADD_TEST(test_out_in_init_high);
        ADD_TEST(test_out_in_init_low);
        ADD_TEST(test_gpio_function_unexported);
        ADD_TEST(test_gpio_function_in);
        ADD_TEST(test_gpio_function_out);

        // event
        ADD_TEST(test_wait_for_edge_rising);
        ADD_TEST(test_wait_for_edge_falling);
        ADD_TEST(test_event_detected_rising);
        ADD_TEST(test_event_detected_falling);
        ADD_TEST(test_event_detected_both);

        // pwm
        if (!pin_data.all_pwms.empty())
        {
            ADD_TEST(test_pwm_multi_duty);
            ADD_TEST(test_pwm_change_frequency);
            ADD_TEST(test_pwm_change_duty_cycle);
            ADD_TEST(test_pwm_cleanup_none);
            ADD_TEST(test_pwm_cleanup_stop);
            ADD_TEST(test_pwm_cleanup_del);
            ADD_TEST(test_pwm_create_all);
        }

#undef ADD_TEST
    }

    void assert(bool b) const
    {
        if (b == false)
            throw std::runtime_error("assert fail");
    }

    void _test_events(int init, GPIO::Edge edge, const std::vector<std::pair<int, bool>>& tests, bool specify_callback,
                      bool use_add_callback)
    {
        bool event_callback_occurred = false;

        GPIO::setmode(GPIO::BOARD);
        GPIO::setup(pin_data.out_a, GPIO::OUT, init);
        GPIO::setup(pin_data.in_a, GPIO::IN);

        TestCallback callback(&event_callback_occurred, std::to_string(pin_data.in_a));

        auto get_saw_event = [&]() mutable -> bool 
        {
            if (specify_callback || use_add_callback)
            {
                bool val = event_callback_occurred;
                event_callback_occurred = false;
                return val;
            }

            return GPIO::event_detected(pin_data.in_a);
        };

        if (specify_callback)
            GPIO::add_event_detect(pin_data.in_a, edge, callback);
        else
            GPIO::add_event_detect(pin_data.in_a, edge);

        if (use_add_callback)
            GPIO::add_event_callback(pin_data.in_a, callback);

        sleep(0.1);
        assert(!get_saw_event());

        for (auto& pair : tests)
        {
            auto& output = pair.first;
            auto& event_expected = pair.second;

            GPIO::output(pin_data.out_a, output);
            sleep(0.1);
            assert(get_saw_event() == event_expected);
            assert(!get_saw_event());
        }

        GPIO::remove_event_detect(pin_data.in_a);
        GPIO::cleanup();
    }
};

int main()
{
    APITests t{};
    t.run();
    return 0;
}
