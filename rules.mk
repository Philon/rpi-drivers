# 指定交叉编译工具链前缀
CROSS_COMPILE ?= /opt/arm-mac-linux-gnueabihf/bin/arm-mac-linux-gnueabihf-

# 指定内核源码目录
KDIR ?= ../linux-rpi-4.19.y

# 指定安装路径（通过scp命令远程拷贝，即scp <target>:<path>）
INSTALL_PATH ?= rpi.local:~/modules

# 检测当前目录下所有测试用例
TESTS ?= $(patsubst %.c, %, $(wildcard *_test.c))

# 检测当前目录下所有设备树
DTBOS ?= $(patsubst %.dts, %.dtbo, $(wildcard *.dts))

# 添加安装文件
INSTALL_FILES += $(obj-m:%.o=%.ko)
INSTALL_FILES += $(DTBOS)
INSTALL_FILES += $(TESTS)

export CC=$(CROSS_COMPILE)gcc
export CROSS_COMPILE
export ARCH=arm

# 各种构建规则
.PHONY: all modules tests install clean

all: modules tests install

modules: $(DTBOS)
	make -C $(KDIR) M=$(PWD) modules

tests: $(TESTS)

install: $(INSTALL_FILES)
	scp -o ConnectTimeout=5 $^ $(INSTALL_PATH)

clean:
	rm -f $(TESTS) $(TESTS:%=%.o) $(DTBOS)
	make -C $(KDIR) M=$(PWD) clean

%_test:%_test.o
	$(CC) $(LDFLAGS) -o $@ $<
%.o:%.c
	$(CC) $(CFLAGS) -c $< -o $@
%.dtbo:%.dts
	$(KDIR)/scripts/dtc/dtc -I dts -o $@ $<