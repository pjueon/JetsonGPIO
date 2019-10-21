AR = ar
RANLIB = ranlib

LIB_NAME = JetsonGPIO
LIB_FULL_NAME = lib$(LIB_NAME).a

SRC_DIR = ../src
INCLUDE_DIR = ../include

LIB_SRCS = JetsonGPIO.cpp
LIB_OBJS = $(LIB_SRCS:%.cpp=%.o)

TEST_SRC = test.cpp

INCLUDE_FLAG = -I$(INCLUDE_DIR)
LIBS_FLAG = -l$(LIB_NAME)

ifeq ($(PREFIX),)
	PREFIX := /usr/local
endif


all : $(LIB_FULL_NAME)
	
$(LIB_FULL_NAME) : $(LIB_OBJS)
	$(AR) rcv $@ $(LIB_OBJS)
	$(RANLIB) $@

$(LIB_OBJS) : $(SRC_DIR)/$(LIB_SRCS) $(INCLUDE_DIR)/$(LIB_NAME).h
	g++ $(INCLUDE_FLAG) -c -o $@ $(SRC_DIR)/$(LIB_SRCS)


install : $(LIB_FULL_NAME)
	install -m 644 $(LIB_FULL_NAME) $(PREFIX)/lib/
	install -m 644 $(INCLUDE_DIR)/$(LIB_NAME).h $(PREFIX)/include/

uninstall : 
	rm -rf $(PREFIX)/lib/$(LIB_FULL_NAME)
	rm -rf $(PREFIX)/include/$(LIB_NAME).h

Test : $(SRC_DIR)/$(TEST_SRC)
	g++ -o $@ $(SRC_DIR)/$(TEST_SRC) $(LIBS_FLAG)


clean :
	rm -rf *.o
	rm -rf *.a

