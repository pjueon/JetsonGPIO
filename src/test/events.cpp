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

#include <iostream>
#include "JetsonGPIO.h"

using namespace std;

int main()
{
// Aside from busy-polling, the library provides three additional ways of monitoring an input event:

// __The wait_for_edge() function__

// This function blocks the calling thread until the provided edge(s) is detected. The function can be called as follows:

GPIO::wait_for_edge(channel, GPIO::RISING);

// The second parameter specifies the edge to be detected and can be GPIO::RISING, GPIO::FALLING or GPIO::BOTH. If you only want to limit the wait to a specified amount of time, a timeout can be optionally set:

// timeout is in milliseconds__
// debounce_time set to 10ms
GPIO::wait_for_edge(channel, GPIO::RISING, 10, 500);

// The function returns the channel for which the edge was detected or 0 if a timeout occurred.

__The event_detected() function__

This function can be used to periodically check if an event occurred since the last call. The function can be set up and called as follows:

```cpp
// set rising edge detection on the channel
GPIO.add_event_detect(channel, GPIO::RISING);
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