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

#include <JetsonGPIO.h>

#include <iostream>
// for delay function.
#include <chrono>
#include <map>
#include <string>
#include <thread>

// for signal handling
#include <signal.h>

using namespace std;

const map<string, int> output_pins{{"JETSON_XAVIER", 18}, {"JETSON_NANO", 33}};

// Pin Definitions
const int output_pin = output_pins.at(GPIO::model);

bool end_this_program = false;

inline void delay(int s) {
  this_thread::sleep_for(chrono::seconds(s));
}

void signalHandler(int s) {
  (void)s;
  end_this_program = true;
}

int main() {
  if (output_pins.find(GPIO::model) == output_pins.end()) {
    cerr << "PWM not supported on this board\n";
    terminate();
  }

  // When CTRL+C pressed, signalHandler will be called
  signal(SIGINT, signalHandler);

  // Pin Setup.
  // Board pin-numbering scheme
  GPIO::setmode(GPIO::BOARD);

  // set pin as an output pin with optional initial state of HIGH
  GPIO::setup(output_pin, GPIO::OUT, GPIO::HIGH);
  GPIO::PWM p(output_pin, 50);
  p.start();

  cout << "PWM running. Press CTRL+C to exit." << endl;

  while (!end_this_program) {
    delay(1);
  }

  p.stop();
  GPIO::cleanup();

  return 0;
}
