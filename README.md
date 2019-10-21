# JetsonGPIO(C++)
A C++ library that enables the use of Jetson's GPIOs  
(Jetson TX1, TX2, AGX Xavier, and Nano)
  
This is a very simple C++ Jetson GPIO Library written by Jueon Park(pjueon).   
It has been written in C++ based on NVIDIA's Jetson-GPIO Python Library.  
(https://github.com/NVIDIA/jetson-gpio)  
It has almost same API with the NVIDIA's python library.   
But it DOESN'T support all functionalites of the NVIDIA's original one.   

**It's not fully tested yet.**  
  
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
sudo cp 99-gpio.rules /etc/udev/rules.d/
```
  
For the new rule to take place, you either need to reboot or reload the udev
rules by running:
```
sudo udevadm control --reload-rules && sudo udevadm trigger
```
  
==========================================================================  


Copyright (c) 2012-2017 Ben Croston <ben@croston.org>.  
Copyright (c) 2019, NVIDIA CORPORATION.   
Copyright (c) 2019 Jueon Park <bluegbg@gmail.com>.  

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
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL  
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER  
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING  
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER  
DEALINGS IN THE SOFTWARE.  

