# JetsonGPIO(C++)
JetsonGPIO(C++) is a C++ port of the **NVIDIA's Jetson.GPIO Python library**(https://github.com/NVIDIA/jetson-gpio).

Jetson TX1, TX2, AGX Xavier, and Nano development boards contain a 40 pin GPIO header, similar to the 40 pin header in the Raspberry Pi. These GPIOs can be controlled for digital input and output using this library. The library provides almost same APIs as the Jetson.GPIO Python library.
  
This document walks through what is contained in The Jetson GPIO library package, how to configure the system and compile the provided sample applications, and the library API.
  
# Package Components
In addition to this document, the JetsonGPIO library package contains the following:
1. The `src` and `include` subdirectories contain the C++ codes that implement all library functionality. The `JetsonGPIO.h` file in the `include` subdirectory is the only header file that should be included into an application and provides the needed APIs. 
2. The `samples` subdirectory contains sample applications to help in getting familiar with the library API and getting started on an application. 
3. The `tests` subdirectory contains test codes to test the library APIs and private utilities that are used to implement the library. Some test codes require specific setups. Check the required setups from the codes before you run them. 


# Installation
### 1. Clone the repository.
```
git clone https://github.com/pjueon/JetsonGPIO
```

### 2. Make build directory and change directory to it. 

```
cd JetsonGPIO
mkdir build && cd build
```

### 3. Configure the cmake
```
cmake .. [OPTIONS]
```

|Option|Default value|Description|
|------|-------------|-----------|
|`-DCMAKE_INSTALL_PREFIX=`|`/usr/local`|installation path|
|`-DBUILD_EXAMPLES=`|ON|build example codes in `samples`|
|`-DJETSON_GPIO_POST_INSTALL=`|ON|run the post install script after installation to set user permissions.|

### 4. Build and Install the library
```
sudo make install
```

# Linking the Library 
**Note**: To build your code with JetsonGPIO, C++11 or above is required.

## Using CMake

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

# Compiling the sample codes 
As mentioned in [Installation](#installation), you can add cmake option `-DBUILD_EXAMPLES=ON` to build example codes in `samples`.
Assuming you are in `build` directory:
```
cmake .. -DBUILD_EXAMPLES=ON
make examples 
```
You can find the compiled results in `build/samples`.


# Library API

The library provides almost same APIs as the the NVIDIA's Jetson GPIO Python library.
The following discusses the use of each API:


#### 1. Include the library

To include the JetsonGPIO use:
```cpp
#include <JetsonGPIO.h>
```

All public APIs are declared in namespace "GPIO". If you want to make your code shorter, you can use:
```cpp
using namespace GPIO; // optional
```

#### 2. Pin numbering

The Jetson GPIO library provides four ways of numbering the I/O pins. The first
two correspond to the modes provided by the RPi.GPIO library, i.e BOARD and BCM
which refer to the pin number of the 40 pin GPIO header and the Broadcom SoC
GPIO numbers respectively. The remaining two modes, CVM and TEGRA_SOC use
strings instead of numbers which correspond to signal names on the CVM/CVB
connector and the Tegra SoC respectively.

To specify which mode you are using (mandatory), use the following function
call:
```cpp
GPIO::setmode(GPIO::BOARD);
// or
GPIO::setmode(GPIO::BCM);
// or
GPIO::setmode(GPIO::CVM);
// or
GPIO::setmode(GPIO::TEGRA_SOC);
```

To check which mode has been set, you can call:
```cpp
GPIO::NumberingModes mode = GPIO::getmode();
```
This function returns an instance of enum class GPIO::NumberingModes. The mode must be one of GPIO::BOARD(GPIO::NumberingModes::BOARD), GPIO::BCM(GPIO::NumberingModes::BCM), GPIO::CVM(GPIO::NumberingModes::CVM), GPIO::TEGRA_SOC(GPIO::NumberingModes::TEGRA_SOC) or GPIO::NumberingModes::None.

#### 3. Warnings

It is possible that the GPIO you are trying to use is already being used
external to the current application. In such a condition, the Jetson GPIO
library will warn you if the GPIO being used is configured to anything but the
default direction (input). It will also warn you if you try cleaning up before
setting up the mode and channels. To disable warnings, call:
```cpp
GPIO::setwarnings(false);
```

#### 4. Set up a channel

The GPIO channel must be set up before use as input or output. To configure
the channel as input, call:
```cpp
// (where channel is based on the pin numbering mode discussed above)
GPIO::setup(channel, GPIO::IN); // channel must be int or std::string
```

To set up a channel as output, call:
```cpp
GPIO::setup(channel, GPIO::OUT);
```

It is also possible to specify an initial value for the output channel:
```cpp
GPIO::setup(channel, GPIO::OUT, GPIO::HIGH);
```


#### 5. Input

To read the value of a channel, use:

```cpp
int value = GPIO::input(channel);
```

This will return either GPIO::LOW(== 0) or GPIO::HIGH(== 1).

#### 6. Output

To set the value of a pin configured as output, use:

```cpp
GPIO::output(channel, state);
```

where state can be GPIO::LOW(== 0) or GPIO::HIGH(== 1).


#### 7. Clean up

At the end of the program, it is good to clean up the channels so that all pins
are set in their default state. To clean up all channels used, call:

```cpp
GPIO::cleanup();
```

If you don't want to clean all channels, it is also possible to clean up
individual channels:

```cpp
GPIO::cleanup(chan1); // cleanup only chan1
```

#### 8. Jetson Board Information and library version

To get information about the Jetson module, use/read:

```cpp
std::string info = GPIO::JETSON_INFO;
// or
std::string info = GPIO::JETSON_INFO();
```

To get the model name of your Jetson device, use/read:

```cpp
std::string model = GPIO::model;
// or
std::string model = GPIO::model();
```

To get information about the library version, use/read:

```cpp
std::string version = GPIO::VERSION;
```

This provides a string with the X.Y.Z version format.

#### 9. Interrupts

Aside from busy-polling, the library provides three additional ways of monitoring an input event:

__The wait_for_edge() function__

This function blocks the calling thread until the provided edge(s) is detected. The function can be called as follows:

```cpp
GPIO::wait_for_edge(channel, GPIO::RISING);
```
The second parameter specifies the edge to be detected and can be GPIO::RISING, GPIO::FALLING or GPIO::BOTH. If you only want to limit the wait to a specified amount of time, a timeout can be optionally set:

```cpp
// timeout is in milliseconds__
// debounce_time set to 10ms
GPIO::WaitResult result = GPIO::wait_for_edge(channel, GPIO::RISING, 10, 500);
```
The function returns a `GPIO::WaitResult` object that contains the channel name for which the edge was detected. 

To check if the event was detected or a timeout occurred, you can use `.is_event_detected()` method of the returned object or just simply cast it to `bool` type:
```cpp
// returns the channel name for which the edge was detected ("None" if a timeout occurred)
std::string eventDetectedChannel = result.channel();

if(result.is_event_detected()){ /*...*/ }
// or 
if(result){ /*...*/ } // is equal to if(result.is_event_detected())
```

__The event_detected() function__

This function can be used to periodically check if an event occurred since the last call. The function can be set up and called as follows:

```cpp
// set rising edge detection on the channel
GPIO::add_event_detect(channel, GPIO::RISING);
run_other_code();
if(GPIO::event_detected(channel))
    do_something();
```

As before, you can detect events for GPIO::RISING, GPIO::FALLING or GPIO::BOTH.

__A callback function run when an edge is detected__

This feature can be used to run a second thread for callback functions. Hence, the callback function can be run concurrent to your main program in response to an edge. This feature can be used as follows:

```cpp
// define callback function
void callback_fn(const std::string& channel) 
{
    std::cout << "Callback called from channel " << channel << std::endl;
}

// add rising edge detection
GPIO::add_event_detect(channel, GPIO::RISING, callback_fn);
```

Any object that satisfies the following requirements can be used as a callback function. 

- Callable with a `const std::string&` type argument (for the channel name) **OR** without any argument. The return type must be `void`.
  - *Note*: If the callback object is **not only** callable with a `const std::string&` type argument **but also** callable without any argument,
  the method with a `const std::string&` type argument will be used as a callback function. 
- Copy-constructible 
- Equality-comparable with same type (ex> func0 == func1)  

Here is a user-defined type callback example:

```cpp
// define callback object
class MyCallback
{
public:
    MyCallback(const std::string& name) : name(name) {}
    MyCallback(const MyCallback&) = default; // Copy-constructible

    void operator()(const std::string& channel) // Callable with one string type argument
    {
        std::cout << "A callback named " << name;
        std::cout << " called from channel " << channel << std::endl;
    }

    bool operator==(const MyCallback& other) const // Equality-comparable
    {
        return name == other.name;
    }

private:
    std::string name;
};

// create callback object
MyCallback my_callback("foo");
// add rising edge detection
GPIO::add_event_detect(channel, GPIO::RISING, my_callback);
```


More than one callback can also be added if required as follows:

```cpp
// you can also use callbacks witout any argument
void callback_one() 
{
    std::cout << "First Callback" << std::endl;
}

void callback_two() 
{
    std::cout << "Second Callback" << std::endl;
}

GPIO::add_event_detect(channel, GPIO::RISING);
GPIO::add_event_callback(channel, callback_one);
GPIO::add_event_callback(channel, callback_two);
```

The two callbacks in this case are run sequentially, not concurrently since there is only one event thread running all callback functions.

In order to prevent multiple calls to the callback functions by collapsing multiple events in to a single one, a debounce time can be optionally set:

```cpp
// bouncetime set in milliseconds
GPIO::add_event_detect(channel, GPIO::RISING, callback_fn, 200);
```

If one of the callbacks are no longer required it may then be removed:

```cpp
GPIO::remove_event_callback(channel, callback_two);
```

Similarly, if the edge detection is no longer required it can be removed as follows:

```cpp
GPIO::remove_event_detect(channel);
```

#### 10. Check function of GPIO channels  

This feature allows you to check the function of the provided GPIO channel:

```cpp
GPIO::Directions direction = GPIO::gpio_function(channel);
```

The function returns either GPIO::IN(GPIO::Directions::IN) or GPIO::OUT(GPIO::Directions::OUT) which are the instances of enum class GPIO::Directions.

#### 11. PWM  

See `samples/simple_pwm.cpp` for details on how to use PWM channels.

The JetsonGPIO library supports PWM only on pins with attached hardware PWM
controllers. Unlike the RPi.GPIO library, the JetsonGPIO library does not
implement Software emulated PWM. Jetson Nano supports 2 PWM channels, and
Jetson AGX Xavier supports 3 PWM channels. Jetson TX1 and TX2 do not support
any PWM channels.

The system pinmux must be configured to connect the hardware PWM controlller(s)
to the relevant pins. If the pinmux is not configured, PWM signals will not
reach the pins! The JetsonGPIO library does not dynamically modify the pinmux
configuration to achieve this. Read the L4T documentation for details on how to
configure the pinmux.


# Using the library from a docker container
The following describes how to use the JetsonGPIO library from a docker container. 

## Preparing the docker image
### From Docker hub
You can get the pre-built image that has JetsonGPIO from [pjueon/jetson-gpio](https://hub.docker.com/r/pjueon/jetson-gpio/).
```shell
docker pull pjueon/jetson-gpio
```

### From the source
You can also build the image from the source. `docker/Dockerfile` is the Dockerfile for the library. 
The following command will build a docker image named `testimg` from it. 

```shell
sudo docker image build -f docker/Dockerfile -t testimg .
```

## Running the container
### Basic options 
You should map `/sys/devices`, `/sys/class/gpio` into the container to access to the GPIO pins.
So you need to add these options to `docker container run` command.
- `-v /sys/devices/:/sys/devices/`
- `-v /sys/class/gpio:/sys/class/gpio`

### Running the container in privilleged mode
The library determines the jetson model by checking `/proc/device-tree/compatible` and `/proc/device-tree/chosen` by default.
These paths only can be mapped into the container in privilleged mode.

The options you need to add are:
- `--privileged`
- `-v /proc/device-tree/compatible:/proc/device-tree/compatible`
- `-v /proc/device-tree/chosen:/proc/device-tree/compatible`

The following example will run `/bin/bash` from the container in privilleged mode. 
```shell
sudo docker container run -it --rm \
--runtime=nvidia --gpus all \
--privileged \
-v /proc/device-tree/compatible:/proc/device-tree/compatible \
-v /proc/device-tree/chosen:/proc/device-tree/chosen \
-v /sys/devices/:/sys/devices/ \
-v /sys/class/gpio:/sys/class/gpio \
pjueon/jetson-gpio /bin/bash
```

### Running the container in non-privilleged mode
If you don't want to run the container in privilleged mode, you can directly provide your jetson model name to the library through the environment variable `JETSON_MODEL_NAME`.

The option you need to add is:
- `-e JETSON_MODEL_NAME=[PUT_YOUR_JETSON_MODEL_NAME_HERE]` (ex> `-e JETSON_MODEL_NAME=JETSON_NANO`)

You can get the proper value for this environment variable by running `samples/jetson_model` in privilleged mode:
```shell
sudo docker container run --rm \
--runtime=nvidia --gpus all \
--privileged \
-v /proc/device-tree/compatible:/proc/device-tree/compatible \
-v /proc/device-tree/chosen:/proc/device-tree/chosen \
-v /sys/devices/:/sys/devices/ \
-v /sys/class/gpio:/sys/class/gpio \
pjueon/jetson-gpio /gpio-cpp/samples/jetson_model
```

The following example will run `/bin/bash` from the container in non-privilleged mode. 

```shell
sudo docker container run -it --rm \
--runtime=nvidia --gpus all \
-v /sys/devices/:/sys/devices/ \
-v /sys/class/gpio:/sys/class/gpio \
-e JETSON_MODEL_NAME=[PUT_YOUR_JETSON_MODEL_NAME_HERE] \
pjueon/jetson-gpio /bin/bash
```
