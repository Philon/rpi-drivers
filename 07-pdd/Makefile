obj-m += leddrv.o
obj-m += leddev.o

KDIR = $(shell pwd)/../../linux-rpi-4.19.y

TEST_SRC := $(wildcard *_test.c)
TEST_OBJ := $(TEST_SRC:%.c=%.o)
TEST_OUT := $(TEST_OBJ:%.o=%)

CLFAGS += -std=gnu99
LFLAGS += -lpthread

export ARCH = arm
export CROSS_COMPILE = /opt/arm-mac-linux-gnueabihf/bin/arm-mac-linux-gnueabihf-

all: module dtbo test install

install: module dtbo
	scp $(obj-m:%.o=%.ko) myled.dtbo rpi.local:~/modules
ifneq ($(TEST_OUT), )
	scp $(TEST_OUT) rpi.local:~/modules
endif

module:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

dtbo:
	../../linux-rpi-4.19.y/scripts/dtc/dtc -I dts -o myled.dtbo myled.dts

test: $(TEST_OUT)
$(TEST_OUT): %:%.o
	$(CROSS_COMPILE)gcc $(LFLAGS) -o $@ $<
$(TEST_OBJ): %.o:%.c
	$(CROSS_COMPILE)gcc $(CLFAGS) -c -o $@ $<

clean:
	$(MAKE) -C $(KDIR) M=`pwd` clean
	rm -f $(TEST_OBJ) $(TEST_OUT)