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

namespace
{
    void NoArguCase()
    {
        std::string f = "ABCdfg 123456";
        auto actual = GPIO::format(f);
        auto expected = f;

        assert::are_equal(expected, actual);
    }

    void EmptyCase()
    {
        std::string f = "";
        auto actual = GPIO::format(f);
        auto expected = f;

        assert::are_equal(expected, actual);
    }

    void NormalCase()
    {
        std::string f = "Hello %s %d";
        auto actual = GPIO::format(f, "world", 3);
        auto expected = "Hello world 3";

        assert::are_equal(expected, actual);
    }

    void NumberCase()
    {
        std::string f = "Number: %05d";
        auto actual = GPIO::format(f, 72);
        auto expected = "Number: 00072";

        assert::are_equal(expected, actual);
    }

    void FloatCase()
    {
        std::string f = "Real Number: %.2f";
        auto actual = GPIO::format(f, 3.141592f);
        auto expected = "Real Number: 3.14";

        assert::are_equal(expected, actual);
    }
} // namespace

int main()
{
    TestSuit suit{};

#define TEST(NAME) {#NAME, NAME}
    suit.add(TEST(NoArguCase));
    suit.add(TEST(EmptyCase));
    suit.add(TEST(NormalCase));
    suit.add(TEST(NumberCase));
    suit.add(TEST(FloatCase));
#undef TEST

    try
    {
        suit.run();
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return -1;
    }

    return 0;
}
