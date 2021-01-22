# JetsonGPIO(C++)
A C++ library that enables the use of Jetson's GPIOs  
(Jetson TX1, TX2, AGX Xavier, and Nano)
    
It's an unofficial port of the NVIDIA's official Jetson GPIO Python library to C++.    
**NVIDIA's official Jetson GPIO Python library**: https://github.com/NVIDIA/jetson-gpio
  
The library provides almost same APIs as the the NVIDIA's Jetson GPIO Python library.
  
**But it DOESN'T support all functionalites of the NVIDIA's original one.**  
(The gpio_event module hasn't implemented yet, for example.)
  
    
**(Tested on Jetson Nano and Jetson TX2)**
 
# Installation
Clone this repository, build it, and install it.

using gnu-make
```
git clone https://github.com/pjueon/JetsonGPIO
cd JetsonGPIO/build
make all
sudo make install
```

using Meson
```
git clone https://github.com/mdegans/JetsonGPIO
meson builddir --default-library static
ninja -C builddir
sudo ninja -C builddir install
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
```
#include <JetsonGPIO>
```
  
All public APIs are declared in namespace "GPIO". If you want to make your code shorter, you can use:  
```
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
```
GPIO::setmode(GPIO::BOARD);
// or
GPIO::setmode(GPIO::BCM);
// or
GPIO::setmode(GPIO::CVM);
// or
GPIO::setmode(GPIO::TEGRA_SOC);
```

To check which mode has be set, you can call:
```
GPIO::NumberingModes mode = GPIO::getmode();
```
This function returns an instance of enum class GPIO::NumberingModes. The mode must be one of GPIO::BOARD(GPIO::NumberingModes::BOARD), GPIO::BCM(GPIO::NumberingModes::BCM), GPIO::CVM(GPIO::NumberingModes::CVM), GPIO::TEGRA_SOC(GPIO::NumberingModes::TEGRA_SOC) or GPIO::NumberingModes::None.

#### 3. Warnings

It is possible that the GPIO you are trying to use is already being used
external to the current application. In such a condition, the Jetson GPIO
library will warn you if the GPIO being used is configured to anything but the
default direction (input). It will also warn you if you try cleaning up before
setting up the mode and channels. To disable warnings, call:
```
GPIO::setwarnings(false);
```

#### 4. Set up a channel

The GPIO channel must be set up before use as input or output. To configure
the channel as input, call:
```
// (where channel is based on the pin numbering mode discussed above)
GPIO::setup(channel, GPIO::IN); // channel must be int or std::string
```

To set up a channel as output, call:
```
GPIO::setup(channel, GPIO::OUT);
```

It is also possible to specify an initial value for the output channel:
```
GPIO::setup(channel, GPIO::OUT, GPIO::HIGH);
```
  

#### 5. Input

To read the value of a channel, use:

```
int value = GPIO::input(channel);
```

This will return either GPIO::LOW(== 0) or GPIO::HIGH(== 1).

#### 6. Output

To set the value of a pin configured as output, use:

```
GPIO::output(channel, state); 
```

where state can be GPIO::LOW(== 0) or GPIO::HIGH(== 1).
  

#### 7. Clean up

At the end of the program, it is good to clean up the channels so that all pins
are set in their default state. To clean up all channels used, call:

```
GPIO::cleanup();
```

If you don't want to clean all channels, it is also possible to clean up
individual channels:

```
GPIO::cleanup(chan1); // cleanup only chan1
```

#### 8. Jetson Board Information and library version

To get information about the Jetson module, use/read:

```
std::string info = GPIO::JETSON_INFO;
```
  
To get the model name of your Jetson device, use/read:

```
std::string model = GPIO::model;
```
  
To get information about the library version, use/read:

```
std::string version = GPIO::VERSION;
```

This provides a string with the X.Y.Z version format.
  
#### 9. Check function of GPIO channels  

This feature allows you to check the function of the provided GPIO channel:

```
GPIO::Directions direction = GPIO::gpio_function(channel);
```

The function returns either GPIO::IN(GPIO::Directions::IN) or GPIO::OUT(GPIO::Directions::OUT) which are the instances of enum class GPIO::Directions.

#### 10. PWM  

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
