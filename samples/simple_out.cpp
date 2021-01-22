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
#include <thread>

// for signal handling
#include <signal.h>

using namespace std;

// Pin Definitions
int output_pin = 18;  // BOARD pin 12, BCM pin 18

bool end_this_program = false;

inline void delay(int s) {
  this_thread::sleep_for(chrono::seconds(s));
}

void signalHandler(int s) {
  (void)s;
  end_this_program = true;
}

int main() {
  // When CTRL+C pressed, signalHandler will be called
  signal(SIGINT, signalHandler);

  // Pin Setup.
  GPIO::setmode(GPIO::BCM);
  // set pin as an output pin with optional initial state of HIGH
  GPIO::setup(output_pin, GPIO::OUT, GPIO::HIGH);

  cout << "Strating demo now! Press CTRL+C to exit" << endl;
  int curr_value = GPIO::HIGH;

  while (!end_this_program) {
    delay(1);
    // Toggle the output every second
    cout << "Outputting " << curr_value << " to pin ";
    cout << output_pin << endl;
    GPIO::output(output_pin, curr_value);
    curr_value ^= GPIO::HIGH;
  }

  GPIO::cleanup();

  return 0;
}
