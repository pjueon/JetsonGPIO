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

#include "private/DictionaryLike.h"
#include "private/test_utility.h"
#include <iostream>
#include <map>
#include <sstream>
#include <vector>

using namespace std::string_literals;

// simple unit test for DictionaryLike class

int main()
{
    std::vector<GPIO::DictionaryLike> testCases = {
        "{ 224: 134, 169: 106 } "s,
        "                  14"s,
        "{169:  PZ.03 }"s,
        "{}"s,
    };

    std::vector<std::string> keys = {"224"s, "169"s, "no_such_key"s};

    std::vector<std::map<std::string, std::string>> expectedResults(testCases.size());

    expectedResults[0]["224"] = "134";
    expectedResults[0]["169"] = "106";
    expectedResults[0]["no_such_key"] = "None";

    expectedResults[1]["224"] = "14";
    expectedResults[1]["169"] = "14";
    expectedResults[1]["no_such_key"] = "14";

    expectedResults[2]["224"] = "None";
    expectedResults[2]["169"] = "PZ.03";
    expectedResults[2]["no_such_key"] = "None";

    expectedResults[3]["224"] = "None";
    expectedResults[3]["169"] = "None";
    expectedResults[3]["no_such_key"] = "None";

    TestSuit suit{};

    for (size_t i = 0; i < testCases.size(); i++)
    {
        for (size_t sub_index = 0; sub_index < keys.size(); sub_index++)
        {
            const auto& key = keys[sub_index];
            auto value = testCases[i].get(key);
            auto expected = expectedResults[i][key];

            std::ostringstream ss{};
            ss << "case " << i << " - " << sub_index;

            auto case_name = ss.str();
            auto func = [expected, value]() { assert::are_equal(expected, value); };
            suit.add({case_name, func});
        }
    }

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