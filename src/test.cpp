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

#include <signal.h>

#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

#include "JetsonGPIO.h"

using namespace std;

// Pin Definitions
const int button_pin = 11; // BOARD pin 11

bool end_this_program = false, cb1 = false, cb2 = false;
atomic_int cb, wb, ewthr;
thread* wthr = nullptr;

inline void delay(int ms)
{
    if (!end_this_program)
        this_thread::sleep_for(chrono::milliseconds(ms));
    if (!end_this_program)
        return;

    // Cleanup and abort
    if (wthr) {
        ewthr = 0;
        cout << "closing wait-event-thread" << endl;
        wthr->join();
        delete wthr;
    }
    GPIO::cleanup();
    exit(0);
}

void signalHandler(int s) { end_this_program = true; }

void callback_fn(int button_pin)
{
    cout << "--Callback called from button_pin " << button_pin << endl;
    ++cb;
}

void callback_one(int button_pin)
{
    cout << "--First Additional Callback" << endl;
    cb1 = true;
}

void callback_two(int button_pin)
{
    cout << "--Second Additional Callback" << endl;
    cb2 = true;
}

/*  Test Events part of the library.
    Tested with a Jetson Nano and the following circuit:
    The button pin connected to a button and a 220pf ceramic capacitor (the other side of this capacitor
    connected to ground) and a 1K resistor (the other end of the resistor is connected to 5V). When the
    button is pressed then the circuit is closed with a direct connection to ground.
    ie. the RISING edge occurs when the button is released.
*/
int testEvents()
{
    GPIO::setup(button_pin, GPIO::IN);

    cout << endl << "### Events Sample Test ###" << endl;
    cout << "#~~ Press CTRL+C to exit" << endl;

    delay(1000);
    cout << endl << "#Demo - GPIO::wait_for_edge" << endl;
    cout << endl << "Waiting for rising edge:" << endl;
    if (GPIO::wait_for_edge(button_pin, GPIO::RISING))
        cout << "--Rising Edge Detected!" << endl;

    for (int i = 0; i < 3; ++i) {
        delay(1000);
        cout << endl << "Waiting for falling edge with a timeout of 2000ms (2 seconds):" << endl;
        if (GPIO::wait_for_edge(button_pin, GPIO::FALLING, 10, 2000)) {
            cout << "--Falling Edge Detected! (try again and wait for timeout)" << endl;
        } else {
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
    cout << endl << "Removed the first additional callback." << endl << "Waiting for rising edge:" << endl;
    while (!cb && !cb2) {
        delay(100);
    }

    delay(1000);
    cout << endl << "Removing event and then readding with one callback function and a bouncetime of 3000 ms" << endl;
    GPIO::remove_event_detect(button_pin);
    cb = 0;
    GPIO::add_event_detect(button_pin, GPIO::RISING, callback_fn, 3000);
    cout << "Press Button 3 times to finish events test! (--need to wait 3 seconds between events!):" << endl;
    while (cb < 3) {
        delay(100);
    }

    GPIO::cleanup(button_pin);

    return 0;
}

void wait_thread(void)
{
    cout << "...blocking_wait-thread begun" << endl;
    for (; ewthr;) {
        // Theres a microsecond or too where this won't be up, but chance of that happenning is very very rare
        // This way allows proper closing of thread easily
        try {
            if (GPIO::wait_for_edge(button_pin, GPIO::RISING, 200, 1000)) {
                cout << "--blocking_wait-thread: Edge Event" << endl;
                ++wb;
            }
        } catch (exception& e) {
            cerr << "[Exception] " << e.what() << " (catched from: wait_thread())" << endl;
        }
    }

    cout << "...blocking_wait-thread finished" << endl;
}

void testEventsConcurrency()
{
    GPIO::setup(button_pin, GPIO::IN);

    cout << endl << "### Concurrent Test Begun ###" << endl;
    cout << endl << "#? Testing event detect whilst blocking_wait-thread is executed concurrently ?#" << endl;

    ewthr = 1;
    wthr = new thread(wait_thread);

    delay(50);
    cout << "...begun new thread with wait_for_edge() concurrently executing" << endl;
    cout << "Press button (expects only blocking_wait-thread-event):" << endl;

    wb = 0;
    while (wb < 1)
        delay(50);

    cout << endl;
    delay(500);

    GPIO::add_event_detect(button_pin, GPIO::RISING, callback_fn, 200);
    cout << "...added event detect with callback on main thread " << endl;
    cout << "Press button (expects blocking_wait-thread-event && callback-event):" << endl;
    cb = 0;
    wb = 0;
    while (cb < 1 && wb < 1)
        delay(50);

    cout << endl;
    delay(500);

    GPIO::event_detected(button_pin);
    cout << "--event_detected() (expects 0):" << GPIO::event_detected(button_pin) << endl;

    cout << endl;
    delay(500);

    GPIO::remove_event_callback(button_pin, callback_fn);
    cout << "...removed callback" << endl;
    cout << "Press button (expects only blocking_wait-thread-event):" << endl;

    wb = 0;
    while (wb < 1)
        delay(50);

    delay(500);
    cout << endl;
    cout << "--event_detected() (expects 1):" << GPIO::event_detected(button_pin) << endl;

    cout << endl;
    delay(500);

    cout << "...removed event" << endl;
    GPIO::remove_event_detect(button_pin);
    cout << "Press button (expects only blocking_wait-thread-event):" << endl;

    wb = 0;
    while (wb < 1)
        delay(50);

    cout << endl;
    delay(500);

    ewthr = 0;
    cout << "...closing blocking_wait-thread..." << endl;
    wthr->join();
    delete wthr;
    wthr = nullptr;

    cout << endl << "#? Testing blocking-event-detect whilst a callback event detect is executed ?#" << endl;

    GPIO::add_event_detect(button_pin, GPIO::RISING, callback_fn);
    cout << "...added event detection callback" << endl;
    cout << "Press button (expects only callback-event):" << endl;

    cb = 0;
    while (cb < 1)
        delay(50);

    cout << endl;
    delay(500);

    cout << "Press button (expects both callback-event and waited-edge-event):" << endl;
    cout << "...beginning wait_for_edge()..." << endl;
    if (GPIO::wait_for_edge(button_pin, GPIO::RISING))
        cout << "--waited-edge-event" << endl;
    else
        cout << "--timeout/error" << endl;

    delay(500);
    cout << endl;
    cout << "--event_detected() (expects 1):" << GPIO::event_detected(button_pin) << endl;
    cout << "--event_detected() (expects 0):" << GPIO::event_detected(button_pin) << endl;

    cout << endl;
    delay(500);

    cout << "Press button (expects only callback-event):" << endl;

    cb = 0;
    while (cb < 1)
        delay(50);

    cout << endl;
    cout << "--event_detected() (expects 1):" << GPIO::event_detected(button_pin) << endl;
    delay(500);

    cout << endl;
    delay(500);

    GPIO::remove_event_callback(button_pin, callback_fn);
    cout << "...removed event detection callback" << endl;
    GPIO::event_detected(button_pin);
    cout << "...beginning wait_for_edge()..." << endl;
    cout << "Press button (expects only waited-edge-event):" << endl;
    if (GPIO::wait_for_edge(button_pin, GPIO::RISING))
        cout << "--waited-edge-event" << endl;
    else
        cout << "--timeout/error" << endl;

    delay(500);
    cout << endl;
    cout << "--event_detected() (expects 1):" << GPIO::event_detected(button_pin) << endl;

    cout << endl;
    delay(500);

    GPIO::remove_event_detect(button_pin);
    cout << "...removed event detection" << endl;
    GPIO::event_detected(button_pin);
    cout << "...beginning wait_for_edge()..." << endl;
    cout << "Press button (expects only waited-edge-event):" << endl;
    if (GPIO::wait_for_edge(button_pin, GPIO::RISING))
        cout << "--waited-edge-event" << endl;
    else
        cout << "--timeout/error" << endl;

    delay(500);
    cout << endl;
    cout << "--event_detected() (expects 0):" << GPIO::event_detected(button_pin) << endl;

    cout << endl;
    delay(500);

    cout << endl << "### Concurrent Test Ended ###" << endl;
    GPIO::cleanup(button_pin);
}

int main()
{
    cout << "model: " << GPIO::model << endl;
    cout << "lib version: " << GPIO::VERSION << endl;
    cout << GPIO::JETSON_INFO << endl;

    int output_pin = 7;
    GPIO::setmode(GPIO::BOARD);
    GPIO::setup(output_pin, GPIO::OUT, GPIO::HIGH);

    cout << "BOARD " << output_pin << "pin, set to OUTPUT, HIGH" << endl;
    cout << "Press Enter to Continue";
    cin.ignore();

    GPIO::output(output_pin, GPIO::LOW);
    cout << output_pin << "pin, set to LOW now" << endl;
    cout << "Press Enter to Continue";
    cin.ignore();

    GPIO::cleanup(output_pin);

    // When CTRL+C pressed, signalHandler will be called
    signal(SIGINT, signalHandler);

    testEvents();
    testEventsConcurrency();

    GPIO::cleanup();
    cout << "end" << endl;
    return 0;
}
