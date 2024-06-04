/*
Copyright (c) 2019-2023, Jueon Park(pjueon) <bluegbgb@gmail.com>.

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

#include "private/TestUtility.h"
#include "JetsonGPIO/TypeTraits.h"
#include <vector>
#include <string>
#include <set>
#include <list>
#include <type_traits>

namespace
{
    void Vector()
    {
        using T = std::vector<double>;
        using expected = double;
        using actual = GPIO::details::value_type_t<T>;

        assert::is_true(std::is_same<expected, actual>::value);
    }

    void Set()
    {
        using T = std::set<std::string>;
        using expected = std::string;
        using actual = GPIO::details::value_type_t<T>;

        assert::is_true(std::is_same<expected, actual>::value);
    }

    void InitializerList()
    {
        using T = std::initializer_list<unsigned int>;
        using expected = unsigned int;
        using actual = GPIO::details::value_type_t<T>;

        assert::is_true(std::is_same<expected, actual>::value);
    }

    void RawArray()
    {
        using T = int[3];
        using expected = int;
        using actual = GPIO::details::value_type_t<T>;

        assert::is_true(std::is_same<expected, actual>::value);
    }

    void List()
    {
        using T = std::set<float>;
        using expected = float;
        using actual = GPIO::details::value_type_t<T>;

        assert::is_true(std::is_same<expected, actual>::value);
    }
}

int main()
{
    TestSuit suit{};

#define TEST(NAME) {#NAME, NAME}
    suit.add(TEST(Vector));
    suit.add(TEST(Set));
    suit.add(TEST(InitializerList));
    suit.add(TEST(RawArray));
    suit.add(TEST(List));
#undef TEST

    return suit.run();
}
