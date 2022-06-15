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

using namespace std::string_literals;

namespace
{
    struct LowerTestCase
    {
        std::string input;
        std::string expected;

        void run()
        {
            auto actual = GPIO::lower(input);
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
    TestSuit suit{};

    std::vector<LowerTestCase> cases = {
        {""s, ""s},
        {"abcd efg 012_xyz 987"s, "abcd efg 012_xyz 987"s},
        {"aBcd eFG 012_XYZ 987"s, "abcd efg 012_xyz 987"s},
        {"    "s, "    "s},
        {"UPPER CASE!!"s, "upper case!!"s},
        {"This Is a Test..."s, "this is a test..."s},
    };

    for (size_t i = 0; i < cases.size(); i++)
    {
        auto name = GPIO::format("case_%02d", i);
        suit.add(cases[i].function(name));
    }

    return suit.run();
}
