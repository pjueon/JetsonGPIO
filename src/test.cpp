/*
Copyright (c) 2012-2017 Ben Croston ben@croston.org.
Copyright (c) 2019, NVIDIA CORPORATION.
Copyright (c) 2019 Jueon Park(pjueon) bluegbg@gmail.com.
Copyright (c) 2021 Adam Rasburn blackforestcheesecake@protonmail.ch.

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

#include <chrono>
#include <iostream>
#include <signal.h>
#include <thread>

#include "JetsonGPIO.h"

using namespace std;

bool end_this_program = false, cb1 = false, cb2 = false;
int cb = 0;

inline void delay(int ms)
{
  if (!end_this_program)
    this_thread::sleep_for(chrono::milliseconds(ms));
  if (!end_this_program)
    return;

  // Cleanup and abort
  GPIO::cleanup();
  exit(0);
}

void signalHandler(int s) { end_this_program = true; }

void callback_fn(int button_pin)
{
  std::cout << "Callback called from button_pin " << button_pin << std::endl;
  ++cb;
}

void callback_one(int button_pin)
{
  std::cout << "First Callback" << std::endl;
  cb1 = true;
}

void callback_two(int button_pin)
{
  std::cout << "Second Callback" << std::endl;
  cb2 = true;
}

int testEvents()
{
  // When CTRL+C pressed, signalHandler will be called
  signal(SIGINT, signalHandler);

  // Pin Definitions
  const int button_pin = 11; // BOARD pin 11

  // Pin Setup.
  GPIO::setmode(GPIO::BOARD);

  // set pin as an output pin with optional initial state of HIGH
  //   GPIO::setup(led_pin, GPIO::OUT, GPIO::LOW);
  GPIO::setup(button_pin, GPIO::IN);

  cout << "Starting Events Test now! Press CTRL+C to exit" << endl;

  delay(1000);
  cout << endl << "#Demo - GPIO::wait_for_edge" << endl;
  cout << endl << "Waiting for rising edge:" << endl;
  GPIO::wait_for_edge(button_pin, GPIO::RISING);
  cout << "--Rising Edge Detected!" << endl;

  for (int i = 0; i < 3; ++i) {
    delay(1000);
    cout << endl << "Waiting for falling edge with a timeout of 2000ms (2 seconds):" << endl;
    if (GPIO::wait_for_edge(button_pin, GPIO::FALLING, 10, 2000)) {
      cout << "--Rising Edge Detected! (try again and wait for timeout)" << endl;
    }
    else {
      cout << "--Timeout Occurred!" << endl;
      break;
    }
  }

  delay(1000);
  cout << endl << "#Demo - GPIO::add_event_detect" << endl;
  cout << endl << "Waiting for rising edge:" << endl;
  GPIO::add_event_detect(button_pin, GPIO::RISING);
  while (!GPIO::event_detected(button_pin)) {
    delay(100);
  }
  cout << "--Rising Edge Detected!" << endl;

  delay(1000);
  cout << endl << "Waiting for rising edge (using callback this time):" << endl;
  GPIO::add_event_detect(button_pin, GPIO::RISING, callback_fn);
  while (!cb) {
    delay(100);
  }

  delay(1000);
  cb = 0;
  GPIO::add_event_callback(button_pin, callback_one);
  GPIO::add_event_callback(button_pin, callback_two);
  cout << endl << "Added 2 more callbacks!" << endl << "Waiting for rising edge:" << endl;
  while (!cb && !cb1 && !cb2) {
    delay(100);
  }

  delay(1);
  cb = cb1 = cb2 = false;
  GPIO::remove_event_callback(button_pin, callback_one);
  cout << endl << "Removed callback 1!" << endl << "Waiting for rising edge:" << endl;
  while (!cb && !cb2) {
    delay(100);
  }

  delay(1000);
  cout << endl << "Removing event and then readding with one callback function and a bouncetime of 3000 ms" << endl;
  GPIO::remove_event_detect(button_pin);
  cb = 0;
  GPIO::add_event_detect(button_pin, GPIO::RISING, callback_fn, 3000);
  cout << "-- Press Button 3 times to exit the application!" << endl;
  while (cb < 3) {
    delay(100);
  }

  return 0;
}

int main()
{
  // cout << "model: " << GPIO::model << endl;
  // cout << "lib version: " << GPIO::VERSION << endl;
  // cout << GPIO::JETSON_INFO << endl;

  // int output_pin = 18;
  // GPIO::setmode(GPIO::BCM);
  // GPIO::setup(output_pin, GPIO::OUT, GPIO::HIGH);

  // cout << "BCM " << output_pin << "pin, set to OUTPUT, HIGH" << endl;
  // cout << "Press Enter to Continue";
  // cin.ignore();

  // GPIO::output(output_pin, GPIO::LOW);
  // cout << output_pin << "pin, set to LOW now" << endl;
  // cout << "Press Enter to Continue";
  // cin.ignore();

  // GPIO::cleanup();

  testEvents();

  cout << "end" << endl;
  return 0;
}
