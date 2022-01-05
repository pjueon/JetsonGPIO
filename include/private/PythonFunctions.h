/*
Copyright (c) 2012-2017 Ben Croston ben@croston.org.
Copyright (c) 2019, NVIDIA CORPORATION.
Copyright (c) 2019 Jueon Park(pjueon) bluegbg@gmail.com.

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

#pragma once
#ifndef PYTHON_FUNCTIONS_H
#define PYTHON_FUNCTIONS_H

#include <string>
#include <vector>
#include <map>
#include <set>
#include <stdexcept>
#include <cstdio>

bool startswith(const std::string& s, const std::string& prefix);

std::vector<std::string> split(const std::string& s, const char d);

bool os_access(const std::string& path, int mode); // os.access

std::vector<std::string> os_listdir(const std::string& path); // os.listdir

bool os_path_isdir(const std::string& path); // os.path.isdirs

bool os_path_exists(const std::string& path); // os.path.exists

std::string strip(const std::string& s);

bool is_None(const std::string& s);

template<class key_t, class element_t>
bool is_in(const key_t& key, const std::map<key_t, element_t>& dictionary)
{
    return dictionary.find(key) != dictionary.end();
}


template<class key_t>
bool is_in(const key_t& key, const std::set<key_t>& set)
{
    return set.find(key) != set.end();
}



// modified from https://stackoverflow.com/questions/2342162/stdstring-formatting-like-sprintf
template<typename ... Args>
std::string format(const std::string& fmt, Args... args)
{
    if(fmt == "" || sizeof...(Args) == 0)
        return fmt;

    int size_s = std::snprintf(nullptr, 0, fmt.c_str(), args...);
    if(size_s <= 0)
        throw std::runtime_error("Error during formatting."); 

    auto size = static_cast<size_t>(size_s);
    
    // In C++11 and later, std::string is guaranteed to be null terminated. 
    // (https://stackoverflow.com/questions/11752705/does-stdstring-have-a-null-terminator)
    // So do not need an extra space for the null-terminator.
    std::string ret(size, '\0');

    // for snprintf, an extra space for the null-terminator MUST be included. (size + 1)
    std::snprintf(&ret[0], size + 1, fmt.c_str(), args...);
    return ret; 
}



#endif // PYTHON_FUNCTIONS_H
