# JetsonGPIO(C++)
A C++ library that enables the use of Jetson's GPIOs  
(Jetson TX1, TX2, AGX Xavier, and Nano)
  
This is a very simple C++ Jetson GPIO Library written by Jueon Park(pjueon).   
It's an unofficial port of the NVIDIA's official Jetson GPIO Python library to C++.
(https://github.com/NVIDIA/jetson-gpio)
  
The library provides almost same APIs as the the NVIDIA's Jetson GPIO Python library.
  
**But it DOESN'T support all functionalites of the NVIDIA's original one.**  
  
    
**Tested on Jetson Nano and Jetson TX2**
 
# Installation
Clone this repository, build it, and install it.
```
git clone https://github.com/pjueon/JetsonGPIO
cd JetsonGPIO/build
make all
sudo make install
```

After installation, you can build your program like this.
```
g++ -o your_program_name your_source_code.cpp -lJetsonGPIO 
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
