
# How to link `JetsonGPIO` to your project
**Note**: To build your code with `JetsonGPIO`, C++11 or higher is required.

## With CMake

Add this to your CMakeLists.txt

```cmake
find_package(JetsonGPIO)
```

assuming you added a target called `mytarget`, then you can link it with:

```cmake
target_link_libraries(mytarget JetsonGPIO::JetsonGPIO)
```

### Without installation

If you don't want to install the library you can add it as an external project with CMake's [`FetchContent`](https://cmake.org/cmake/help/latest/module/FetchContent.html)(cmake 3.11 or higher is required):

```cmake
include(FetchContent)
FetchContent_Declare(
  JetsonGPIO 
  GIT_REPOSITORY https://github.com/pjueon/JetsonGPIO.git 
  GIT_TAG master
)
FetchContent_MakeAvailable(JetsonGPIO)

target_link_libraries(mytarget JetsonGPIO::JetsonGPIO)
```

The code will be automatically fetched at configure time and built alongside your project.

Note that with this method will *not* set user permissions, so you will need to set user permissions manually or run your code with root permissions.

To set user permissions, run `scripts/post_install.sh` script. 
Assuming you are in `build` directory:
```
sudo bash ../scripts/post_install.sh
```


## Without CMake
- The library name is `JetsonGPIO`.
- The library header files are installed in `/usr/local/include` by default.
- The library has dependency on `pthread` (the library uses `std::thread`)

The following example shows how to compile your code with the library: 
```
g++ -o your_program_name [your_source_codes...] -lJetsonGPIO -lpthread
```
