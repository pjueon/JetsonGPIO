#include "private/DictionaryLike.h"
#include <iostream>
#include <vector>
#include <map>

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

    std::vector<std::string> keys = 
    {
        "224"s, "169"s, "no_such_key"s
    };

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


    for(size_t i = 0; i < testCases.size(); i++)
    {
        size_t sub_index = 0;
        for(const auto& key : keys)
        {
            auto value = testCases[i].get(key);
            auto expected = expectedResults[i][key];

            std::cout << "case " << i <<" - " << sub_index << ": ";
            if (value == expected)
                std::cout << "PASSED";
            else
                std::cout << "FAILED (expected: " << expected << ", actual: " << value << ")";
            
            std::cout << std::endl;

            sub_index++;
        }
    }

    return 0;
}