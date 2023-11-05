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

namespace
{
    struct NotIterable
    {
    };


    void Vector()
    {
        using T = std::vector<double>;
        assert::is_true(GPIO::details::is_iterable_v<T>);
    }

    void Set()
    {
        using T = std::set<float>;
        assert::is_true(GPIO::details::is_iterable_v<T>);
    }

    void InitializerList()
    {
        using T = std::initializer_list<std::string>;
        assert::is_true(GPIO::details::is_iterable_v<T>);
    }

    void RawArray()
    {
        using T = std::string[10];
        assert::is_true(GPIO::details::is_iterable_v<T>);
    }

    void List()
    {
        using T = std::list<int>;
        assert::is_true(GPIO::details::is_iterable_v<T>);
    }    

    void NotIterableType()
    {
        using T = NotIterable;
        assert::is_false(GPIO::details::is_iterable_v<T>);
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
    suit.add(TEST(NotIterableType));
#undef TEST

    return suit.run();
}
