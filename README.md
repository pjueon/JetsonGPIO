# JetsonGPIO(C++)
JetsonGPIO(C++) is an C++ port of the **NVIDIA's Jetson.GPIO Python library**(https://github.com/NVIDIA/jetson-gpio).    

Jetson TX1, TX2, AGX Xavier, and Nano development boards contain a 40 pin GPIO header, similar to the 40 pin header in the Raspberry Pi. These GPIOs can be controlled for digital input and output using this library. The library provides almost same APIs as the Jetson.GPIO Python library.  
  

# Installation
Clone this repository, build it, and install it.
```
git clone https://github.com/pjueon/JetsonGPIO
cd JetsonGPIO/build
make all
sudo make install
```


# Setting User Permissions

In order to use the Jetson GPIO Library, the correct user permissions/groups must  
be set first. Or you have to run your program with root permission.    

Create a new gpio user group. Then add your user to the newly created group.  
```
sudo groupadd -f -r gpio
sudo usermod -a -G gpio your_user_name
```
Install custom udev rules by copying the 99-gpio.rules file into the rules.d  
directory. The 99-gpio.rules file was copied from NVIDIA's official repository.  

```
sudo cp JetsonGPIO/99-gpio.rules /etc/udev/rules.d/
```

For the new rule to take place, you either need to reboot or reload the udev
rules by running:
```
sudo udevadm control --reload-rules && sudo udevadm trigger
```

# Library API

The library provides almost same APIs as the the NVIDIA's Jetson GPIO Python library.
The following discusses the use of each API:  

#### 1. Include the libary

To include the JetsonGPIO use:
```cpp
#include <JetsonGPIO>
```

All public APIs are declared in namespace "GPIO". If you want to make your code shorter, you can use:  
```cpp
using namespace GPIO; // optional
```

To compile your program use:
```
g++ -o your_program_name your_source_code.cpp -lJetsonGPIO
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

To check which mode has be set, you can call:
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
```

To get the model name of your Jetson device, use/read:

```cpp
std::string model = GPIO::model;
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
GPIO::wait_for_edge(channel, GPIO::RISING, 10, 500);
```

The function returns the channel for which the edge was detected or 0 if a timeout occurred.


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
void callback_fn(int channel) {
    std::cout << "Callback called from channel " << channel << std::endl;
}

// add rising edge detection
GPIO::add_event_detect(channel, GPIO::RISING, callback_fn);
```

More than one callback can also be added if required as follows:

```cpp
void callback_one(int channel) {
    std::cout << "First Callback" << std::endl;
}

void callback_two(int channel) {
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

If one of the callbacks are no longer required it may then be removed

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
