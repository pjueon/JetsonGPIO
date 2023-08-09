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

namespace
{
    struct NotIterable
    {
    };


    void Vector()
    {
        std::vector<double> data{};
        assert::is_true(GPIO::details::is_iterable_v<decltype(data)>);
    }

    void Set()
    {
        std::set<std::string> data{"foo", "bar", "ok"};
        assert::is_true(GPIO::details::is_iterable_v<decltype(data)>);
    }

    void InitializerList()
    {
        auto data = {1, 2, 3};
        assert::is_true(GPIO::details::is_iterable_v<decltype(data)>);
    }

    void RawArray()
    {
        std::string data[] = {"hello", "world", "!"};
        assert::is_true(GPIO::details::is_iterable_v<decltype(data)>);
    }

    void NotIterableType()
    {
        NotIterable data{};
        assert::is_false(GPIO::details::is_iterable_v<decltype(data)>);
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
    suit.add(TEST(NotIterableType));
#undef TEST

    return 0;
}
