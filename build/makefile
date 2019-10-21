AR = ar
RANLIB = ranlib

LIB_NAME = JetsonGPIO
LIB_FULL_NAME = lib$(LIB_NAME).a

SRC_DIR = ../src
INCLUDE_DIR = ../include
LIB_DIR = ../lib



LIB_SRCS = JetsonGPIO.cpp
LIB_OBJS = $(LIB_SRCS:%.cpp=%.o)

TEST_SRC = test.cpp

INCLUDE_FLAG = -I$(INCLUDE_DIR)
LIBS_FLAG = -l$(LIB_NAME)
LIB_DIR_FLAG = -L../lib


all : $(LIB_FULL_NAME)
	
$(LIB_FULL_NAME) : $(LIB_OBJS)
	$(AR) rcv $(LIB_DIR)/$@ $(LIB_OBJS)
	$(RANLIB) $(LIB_DIR)/$@

$(LIB_OBJS) : $(SRC_DIR)/$(LIB_SRCS) $(SRC_DIR)/JetsonGPIO.h
	g++ $(INCLUDE_FLAG) -c -o $@ $(SRC_DIR)/$(LIB_SRCS)

Test :  $(SRC_DIR)/$(TEST_SRC)
	g++ $(INCLUDE_FLAG) $(LIB_DIR_FLAG) -o $@ $(SRC_DIR)/$(TEST_SRC) $(LIBS_FLAG)


$(SRC_DIR)/JetsonGPIO.h :

$(SRC_DIR)/main.cpp :


clean :
	rm -rf *.o

