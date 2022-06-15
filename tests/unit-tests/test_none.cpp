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
#include <string>
#include <type_traits>

using namespace std::string_literals;

namespace
{
    void integer_equal() { assert::are_equal(-1, GPIO::None); }

    void integer_conversion()
    {
        constexpr bool convertible = std::is_convertible<GPIO::NoneType, int>::value;
        assert::is_true(convertible);
    }

    void string_eqaul() { assert::are_equal("None"s, (std::string)GPIO::None); }

    void string_conversion()
    {
        constexpr bool convertible = std::is_convertible<GPIO::NoneType, std::string>::value;
        assert::is_true(convertible);
    }

    void is_not_none0()
    {
        bool b = GPIO::is_None("-1");
        assert::is_false(b);
    }

    void is_not_none1()
    {
        bool b = GPIO::is_None("");
        assert::is_false(b);
    }

    void is_not_none2()
    {
        bool b = GPIO::is_None("none");
        assert::is_false(b);
    }

    void is_not_none3()
    {
        bool b = GPIO::is_None(" None");
        assert::is_false(b);
    }

    void is_not_none4()
    {
        bool b = GPIO::is_None("None ");
        assert::is_false(b);
    }

    void is_not_none5()
    {
        bool b = GPIO::is_None(0);
        assert::is_false(b);
    }

    void is_none0()
    {
        bool b = GPIO::is_None(-1);
        assert::is_true(b);
    }

    void is_none1()
    {
        bool b = GPIO::is_None("None");
        assert::is_true(b);
    }
} // namespace

int main()
{
    TestSuit suit{};

#define TEST(NAME) {#NAME, NAME}

    suit.add(TEST(integer_equal));
    suit.add(TEST(integer_conversion));
    suit.add(TEST(string_eqaul));
    suit.add(TEST(string_conversion));

    suit.add(TEST(is_not_none0));
    suit.add(TEST(is_not_none1));
    suit.add(TEST(is_not_none2));
    suit.add(TEST(is_not_none3));
    suit.add(TEST(is_not_none4));
    suit.add(TEST(is_not_none5));

    suit.add(TEST(is_none0));
    suit.add(TEST(is_none1));

#undef TEST

    return suit.run();
}
