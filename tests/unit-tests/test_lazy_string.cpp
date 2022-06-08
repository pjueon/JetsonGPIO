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

#include "JetsonGPIO.h"
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

#define TEST(F)                                                                                                        \
    std::cout << "case name: " << #F << ", result: ";                                                                  \
    {                                                                                                                  \
        bool passed = F();                                                                                             \
        std::cout << (passed ? "PASSED" : "FAILED") << std::endl;                                                      \
        all_passed = all_passed && passed;                                                                             \
    }

int main()
{
    bool all_passed = true;

    // TEST([]() { return false; });

    TEST(compare_to_const_char0);
    TEST(compare_to_const_char1);
    TEST(compare_to_const_char2);
    TEST(compare_to_const_char3);
    TEST(compare_to_lazy_string0);
    TEST(compare_to_lazy_string1);
    TEST(compare_to_string0);
    TEST(compare_to_string1);
    TEST(compare_to_string2);
    TEST(compare_to_string3);
    TEST(lazy_evaluation0);
    TEST(lazy_evaluation1);
    TEST(cache);

    std::cout << "total: " << (all_passed ? "PASSED" : "FAILED") << std::endl;  
    return all_passed ? 0 : -1;
}