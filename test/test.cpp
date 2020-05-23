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


/* 
 * File:   main.cpp
 * Author: Josh -- JKI757
 * 
 * modified test to include PWM
 *
 * Created on May 21, 2020, 6:00 PM
 */

#include <cstdlib>
#include "JetsonGPIO.h"
#include <memory>

#include <iostream>
#include <chrono> 
#include <thread>

// for signal handling
#include <signal.h>


#define JETSON_STEER_PIN 32 //GPIO07 //pin32
#define JETSON_DRIVE_PIN 33 // GPIO13 //pin33
#define JETSON_IN1_PIN 29   // GPIO01 //pin 29, gpio5
#define JETSON_IN2_PIN 31   //GPIO06 //pin 31, gpio6

bool end_this_program = false;

void signalHandler (int s){
	end_this_program = true;
}
inline void delay(int s){
	std::this_thread::sleep_for(std::chrono::seconds(s));
}



/*
 * 
 */
int main(int argc, char** argv) {
    signal(SIGINT, signalHandler);

    std::shared_ptr<GPIO::PWM> drive_pwm;
    std::shared_ptr<GPIO::PWM> steer_pwm;

    std::cout << "model: "<< GPIO::model << std::endl;
    std::cout << "lib version: " << GPIO::VERSION << std::endl;
    std::cout << GPIO::JETSON_INFO << std::endl;

        
    GPIO::setmode(GPIO::BOARD);
    GPIO::setup(JETSON_DRIVE_PIN, GPIO::OUT, GPIO::HIGH);
    drive_pwm = std::make_shared<GPIO::PWM>(JETSON_DRIVE_PIN, 50);
    steer_pwm = std::make_shared<GPIO::PWM>(JETSON_STEER_PIN, 1000);
   
    drive_pwm->start(.5);
    steer_pwm->start(.5);
    
    std::cout << "Starting demo now! Press CTRL+C to exit" << std::endl;
    int curr_value = GPIO::HIGH;

    GPIO::setup(JETSON_IN1_PIN, GPIO::OUT,GPIO::LOW);
    GPIO::setup(JETSON_IN2_PIN, GPIO::OUT,GPIO::LOW);

    while(!end_this_program){
        delay(1);

        std::cout << "Outputting " << curr_value << " to pin ";
        std::cout << JETSON_IN1_PIN << std::endl;
        std::cout << "Outputting " << curr_value << " to pin ";
        std::cout << JETSON_IN2_PIN << std::endl;
        GPIO::output(JETSON_IN1_PIN, curr_value);
        GPIO::output(JETSON_IN2_PIN, curr_value);
        
        curr_value ^= GPIO::HIGH;

        
    }
    
    drive_pwm->stop();
    steer_pwm->stop();
    
    GPIO::cleanup();

    return 0;
}

