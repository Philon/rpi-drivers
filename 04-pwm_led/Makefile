# 模块驱动，必须以obj-m=xxx形式编写
obj-m = pwmled.o

KDIR = $(shell pwd)/../../linux-rpi-4.19.y

TEST_SRC := $(wildcard *_test.c)
TEST_OBJ := $(TEST_SRC:%.c=%.o)
TEST_OUT := $(TEST_OBJ:%.o=%)

CLFAGS += -std=c99

export ARCH = arm
export CROSS_COMPILE = /opt/arm-mac-linux-gnueabihf/bin/arm-mac-linux-gnueabihf-

all: module test install

install: module
	scp $(obj-m:%.o=%.ko) rpi.local:~/modules
	scp $(TEST_OUT) rpi.local:~/modules

module:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

test: $(TEST_OUT)
$(TEST_OUT): %:%.o
	$(CROSS_COMPILE)gcc $(LFLAGS) -o $@ $<
$(TEST_OBJ): %.o:%.c
	$(CROSS_COMPILE)gcc $(CLFAGS) -c -o $@ $<

clean:
	$(MAKE) -C $(KDIR) M=`pwd` clean
	rm -f $(TEST_OBJ) $(TEST_OUT)