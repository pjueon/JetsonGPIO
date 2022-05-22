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
    GPIO::LazyString a __attribute__((unused)) {[&is_evaluated]() -> std::string
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

#define TEST(F) std::cout << "test: " << #F << ", result: " << (F() ? "PASSED" : "FAILED") << std::endl;

int main()
{
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

    return 0;
}