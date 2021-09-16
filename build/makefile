# Copyright (c) 2012-2017 Ben Croston ben@croston.org.
# Copyright (c) 2019, NVIDIA CORPORATION.
# Copyright (c) 2019 Jueon Park(pjueon) bluegbg@gmail.com.

# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.

AR = ar
RANLIB = ranlib

LIB_NAME = JetsonGPIO
LIB_FULL_NAME = lib$(LIB_NAME).a

SRC_DIR = ../src
INCLUDE_DIR = ../include

LIB_SRCS = JetsonGPIO.cpp PythonFunctions.cpp gpio_pin_data.cpp gpio_event.cpp
LIB_HEADERS = $(INCLUDE_DIR)/$(LIB_NAME).h $(INCLUDE_DIR)/private/PythonFunctions.h $(INCLUDE_DIR)/private/gpio_pin_data.h  $(INCLUDE_DIR)/private/gpio_event.h
LIB_OBJS = $(LIB_NAME).o PythonFunctions.o gpio_pin_data.o gpio_event.o

TEST_SRC = test.cpp

INCLUDE_FLAG = -I$(INCLUDE_DIR)
LIBS_FLAG = -l$(LIB_NAME)

ifeq ($(PREFIX),)
	PREFIX := /usr/local
endif

all : $(LIB_OBJS)
	$(AR) rcv $(LIB_FULL_NAME) $(LIB_OBJS)
	$(RANLIB) $(LIB_FULL_NAME)

$(LIB_NAME).o : $(SRC_DIR)/$(LIB_NAME).cpp $(LIB_HEADERS)
	g++ $(INCLUDE_FLAG) -c -o $@ $(SRC_DIR)/$(LIB_NAME).cpp

PythonFunctions.o :  $(SRC_DIR)/PythonFunctions.cpp $(INCLUDE_DIR)/private/PythonFunctions.h
	g++ $(INCLUDE_FLAG) -c -o $@ $(SRC_DIR)/PythonFunctions.cpp

gpio_pin_data.o : $(SRC_DIR)/gpio_pin_data.cpp $(LIB_HEADERS)
	g++ $(INCLUDE_FLAG) -c -o $@ $(SRC_DIR)/gpio_pin_data.cpp

gpio_event.o : $(SRC_DIR)/gpio_event.cpp $(LIB_HEADERS)
	g++ $(INCLUDE_FLAG) -I -lpthread -c -o $@ $(SRC_DIR)/gpio_event.cpp

install : $(LIB_FULL_NAME)
	install -m 644 $(LIB_FULL_NAME) $(PREFIX)/lib/
	install -m 644 $(INCLUDE_DIR)/$(LIB_NAME).h $(PREFIX)/include/

uninstall : 
	rm -rf $(PREFIX)/lib/$(LIB_FULL_NAME)
	rm -rf $(PREFIX)/include/$(LIB_NAME).h

test : $(SRC_DIR)/$(TEST_SRC)
	g++ -o $@ $(SRC_DIR)/$(TEST_SRC) $(LIBS_FLAG) -lpthread

clean :
	rm -rf  *.o
	rm -rf  *.a

