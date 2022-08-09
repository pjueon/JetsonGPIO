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

#include "private/PythonFunctions.h"
#include "private/TestUtility.h"
#include <iostream>
#include <vector>

using namespace std::string_literals;

namespace
{
    struct TestCase
    {
        std::string input;
        std::string prefix;
        bool expected;

        void run()
        {
            auto actual = GPIO::startswith(input, prefix);
            assert::are_equal(expected, actual);
        }

        TestFunction function(const std::string& name)
        {
            return {name, [this]() { run(); }};
        }
    };
} // namespace

int main()
{
    std::vector<TestCase> cases = {
        {"ABcde xyz987"s, "abc"s, false},
        {"jetson nano"s, "jetson"s, true},
        {""s, ""s, true},
        {"xyz 012345"s, ""s, true},
        {"  009124ab xyz"s, "0"s, false},
        {"Jetson GPIO Test 1234"s, "Jetson GPIO Test"s, true},
        {"Jetson GPIO Test 1234"s, "Jetson GPIO Test 5678"s, false},
    };

    TestSuit suit{};

    for (size_t i = 0; i < cases.size(); i++)
    {
        auto name = GPIO::format("case_%02d", i);
        suit.add(cases[i].function(name));
    }

    return suit.run();
}
