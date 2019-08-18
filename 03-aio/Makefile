# 模块驱动，必须以obj-m=xxx形式编写
obj-m = gpiokey.o

KDIR = $(shell pwd)/../../linux-rpi-4.19.y

TEST := gpiokey_test
TEST_SRC := $(TEST).c
TEST_OBJ := $(TEST).o

export ARCH = arm
export CROSS_COMPILE = /opt/arm-mac-linux-gnueabihf/bin/arm-mac-linux-gnueabihf-

all: module

module:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

test: 
	$(CROSS_COMPILE)gcc $(CFLAGS) -c $(TEST_SRC) -o $(TEST_OBJ)
	$(CROSS_COMPILE)gcc $(LFLAGS) -o $(TEST) $(TEST_OBJ)

clean:
	$(MAKE) -C $(KDIR) M=`pwd` clean
	rm -f $(TEST_OBJ) $(TEST)