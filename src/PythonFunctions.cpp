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

#include "private/PythonFunctions.h"

#include <dirent.h>
#include <unistd.h>
#include <cctype>
#include <sstream>

using namespace std;

bool startswith(const string& s, const string& prefix) {
  size_t pre_size = prefix.size();
  if (s.size() < pre_size)
    return false;

  return prefix == s.substr(0, prefix.size());
}

vector<string> split(const string& s, const char d) {
  stringstream buffer(s);

  string tmp;
  vector<string> outputVector;

  while (getline(buffer, tmp, d))
    outputVector.push_back(tmp);

  return outputVector;
}

bool os_access(const string& path, int mode) {  // os.access
  return access(path.c_str(), mode) == 0;
}

vector<string> os_listdir(const string& path) {  // os.listdir
  DIR* dir;
  struct dirent* ent;
  vector<string> outputVector;

  if ((dir = opendir(path.c_str())) != nullptr) {
    while ((ent = readdir(dir)) != nullptr) {
      outputVector.emplace_back(ent->d_name);
    }
    closedir(dir);
    return outputVector;
  } else {
    throw runtime_error("could not open directory: " + path);
  }
}

bool os_path_exists(const string& path) {  // os.path.exists
  return os_access(path, F_OK);
}

string strip(const string& s) {
  int start_idx = 0;
  int total = s.size();
  int end_idx = total - 1;
  for (; start_idx < total; start_idx++) {
    if (!isspace(s[start_idx]))
      break;
  }
  if (start_idx == total)
    return "";
  for (; end_idx > start_idx; end_idx--) {
    if (!isspace(s[end_idx]))
      break;
  }
  return s.substr(start_idx, end_idx - start_idx + 1);
}
