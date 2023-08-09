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

#include "JetsonGPIO.h"
#include "private/TestUtility.h"
#include <iostream>
#include <map>
#include <string>
#include <vector>

using namespace std::string_literals;

constexpr auto sample_string = "ABCD 12345";

bool compare_to_const_char0()
{
    GPIO::LazyString a = sample_string;
    return sample_string == a;
}

bool compare_to_const_char1()
{
    GPIO::LazyString a = sample_string;
    return a == sample_string;
}

bool compare_to_const_char2()
{
    GPIO::LazyString a = sample_string;
    return "foo" != a;
}

bool compare_to_const_char3()
{
    GPIO::LazyString a = sample_string;
    return a != "foo";
}

bool compare_to_lazy_string0()
{
    GPIO::LazyString a = sample_string;
    GPIO::LazyString b{[]() -> std::string { return sample_string; }};
    return a == b;
}

bool compare_to_lazy_string1()
{
    GPIO::LazyString a = sample_string;
    GPIO::LazyString b{[]() -> std::string { return "foo"; }};
    return a != b;
}

bool compare_to_string0()
{
    GPIO::LazyString a = sample_string;
    std::string b = sample_string;
    return a == b;
}

bool compare_to_string1()
{
    GPIO::LazyString a = sample_string;
    std::string b = sample_string;
    return b == a;
}

bool compare_to_string2()
{
    GPIO::LazyString a = sample_string;
    std::string b = "foo";
    return a != b;
}

bool compare_to_string3()
{
    GPIO::LazyString a = sample_string;
    std::string b = "foo";
    return b != a;
}

bool lazy_evaluation0()
{
    bool is_evaluated = false;
    GPIO::LazyString a{[&is_evaluated]() -> std::string
                       {
                           is_evaluated = true;
                           return sample_string;
                       }};

    a();
    return is_evaluated;
}

bool lazy_evaluation1()
{
    bool is_evaluated = false;
    GPIO::LazyString a __attribute__((unused)){[&is_evaluated]() -> std::string
                                               {
                                                   is_evaluated = true;
                                                   return sample_string;
                                               }};

    return !is_evaluated;
}

bool cache()
{
    int count = 0;
    GPIO::LazyString a{[&count]() -> std::string
                       {
                           count++;
                           return sample_string;
                       }};

    for (int i = 0; i < 10; i++)
    {
        a();
    }

    return count == 1;
}

int main()
{
    TestSuit suit{};
    suit.reserve(20);

#define TEST(NAME) {#NAME, NAME}
    suit.add(TEST(compare_to_const_char0));
    suit.add(TEST(compare_to_const_char1));
    suit.add(TEST(compare_to_const_char2));
    suit.add(TEST(compare_to_const_char3));
    suit.add(TEST(compare_to_lazy_string0));
    suit.add(TEST(compare_to_lazy_string1));
    suit.add(TEST(compare_to_string0));
    suit.add(TEST(compare_to_string1));
    suit.add(TEST(compare_to_string2));
    suit.add(TEST(compare_to_string3));
    suit.add(TEST(lazy_evaluation0));
    suit.add(TEST(lazy_evaluation1));
    suit.add(TEST(cache));
#undef TEST

    return suit.run();
}
