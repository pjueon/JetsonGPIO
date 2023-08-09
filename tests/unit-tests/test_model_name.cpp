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

#include "private/Model.h"
#include "private/ModelUtility.h"
#include "private/TestUtility.h"
#include <iostream>

int main()
{
    TestSuit suit{};

#define TEST(NAME)                                                                                                     \
    {                                                                                                                  \
        "model to name " #NAME, []() { assert::are_equal(#NAME, GPIO::model_name(GPIO::NAME)); }                       \
    }
    suit.add(TEST(CLARA_AGX_XAVIER));
    suit.add(TEST(JETSON_NX));
    suit.add(TEST(JETSON_XAVIER));
    suit.add(TEST(JETSON_TX2));
    suit.add(TEST(JETSON_TX1));
    suit.add(TEST(JETSON_NANO));
    suit.add(TEST(JETSON_TX2_NX));
    suit.add(TEST(JETSON_ORIN));
#undef TEST

#define TEST(NAME)                                                                                                     \
    {                                                                                                                  \
        "name to model " #NAME, []() { assert::is_true(GPIO::NAME == GPIO::name_to_model(#NAME)); }                    \
    }
    suit.add(TEST(CLARA_AGX_XAVIER));
    suit.add(TEST(JETSON_NX));
    suit.add(TEST(JETSON_XAVIER));
    suit.add(TEST(JETSON_TX2));
    suit.add(TEST(JETSON_TX1));
    suit.add(TEST(JETSON_NANO));
    suit.add(TEST(JETSON_TX2_NX));
    suit.add(TEST(JETSON_ORIN));
#undef TEST

    return suit.run();
}
