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

#include "private/TestUtility.h"
#include <iostream>
#include <stdexcept>

namespace assert
{
    void is_true(bool b, std::string msg)
    {
        if (b)
            return;

        if (msg != "")
            msg = " message: " + msg;

        throw std::runtime_error("assert failed." + msg);
    }

    void expect_exception(const std::function<void(void)>& func, std::string msg)
    {
        bool exception_occured = false;

        try
        {
            func();
        }
        catch (...)
        {
            exception_occured = true;
        }

        is_true(exception_occured, msg);
    }
} // namespace assert

void TestFunction::execute() const
{
    if (func != nullptr)
        func();
}

void TestSuit::run()
{
    setup();
    std::cout << "Number of test cases: " << tests.size() << std::endl;

    for (auto& test : tests)
    {
        std::cout << "Testing " << test.name << std::endl;

        try
        {
            test.func();
        }
        catch (...)
        {
            on_failed();
            std::cout << "test failed." << std::endl;
            throw;
        }
    }

    std::cout << "All tests passed." << std::endl;
}

void TestSuit::reserve(size_t n) { tests.reserve(n); }

void TestSuit::add(const TestFunction& test) { tests.emplace_back(test); }
