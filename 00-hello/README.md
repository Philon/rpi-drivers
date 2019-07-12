# 00 HelloWorld

本文源码：https://github.com/philon/rpi-drivers/tree/master/00-hello

最近打算系统学习一下嵌入式Linux驱动开发，之前尝试过qemu模拟器搭建的ARM平台，但却花了大量精力来学习如何使用qemu工具，感觉有些本末倒置了。加之手上的教材更偏理论，学起来比较乏味。正好手里有块树莓派3B+，以及非常多的扩展模块！！！我心想，何不尝试着把这些模块驱动都写一写呢，反正都是驱动开发，用个实际的硬件岂不更好。

我计划实现LED、温湿度传感器、陀螺仪、PS遥感、红外模组、超声波模组、光敏传感器等驱动，其目的是为了学习嵌入式Linux驱动开发的思想，以及常见的接口及模块的原理。我会把整个学习过程整理为《树莓派驱动开发实战》。当然，我的初衷是学习ARM-Linux驱动开发，如果不是必须，我不会可以强调硬件平台，而是更侧重于驱动编程和电路设计。

![](https://i.loli.net/2019/07/12/5d28aa437927829767.jpg)

# 准备工作

我的开发环境如下：
1. 宿主机：华硕A55VM | Ubuntu18.04
2. 开发板：Raspberry 3 Model B+ 2017
3. 编辑器：Visual Studio Code | C/C++扩展

由于文章中会存在大量命令脚本的引用，先做个声明：
```sh
philon@a55v:~$ # 表示在宿主机敲出的命令
philon@rpi:~$  # 表示在开发板敲出的命令
```

此外，如果不是必须，我不会拿串口调试开发板，一律ssh！

在正式开发之前还有几个准备工作：
1. 确定开发板运行的内核版本
```sh
philon@rpi:~ $ uname -a
Linux rpi 4.19.42-v7+ #1219 SMP Tue May 14 21:20:58 BST 2019 armv7l GNU/Linux
```
内核版本是4.19.42，注意最后的**armv7**，尽管RPi-3B+的处理器平台是64位4核，不过它的操作系统是32位的，这一点很重要，因为关乎接下来的编译器选择。

2. 搭建交叉编译环境
```sh
philon@a55v:~$ sudo apt install gcc-arm-linux-gnueabihf
```
正如上面所说，我选择的交叉编译器的是`arm-linux-gnueabihf-gcc`(32位)，而不是`aarch64-linux-gcc`(64位)。

# 宿主机环境搭建

正如之前确认的，内核版本是`Linux rpi 4.19.42-v7+`，因此下载的内核源码大版本号要一致：

```sh
# 下载内核源码
philon@a55v:~$ wget https://github.com/raspberrypi/linux/archive/rpi-4.19.y.tar.gz
# 解压，进入内核目录
philon@a55v:~$ tar xvf rpi-4.19.y.tar.gz && cd linux-rpi-4.19.y
# 清理内核
philon@a55v:~/linux-rpi-4.19.y$ make mrproper
# 加载RPi-3B+的配置
philon@a55v:~/linux-rpi-4.19.y$ make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- bcm2709_defconfig
# 构建
philon@a55v:~/linux-rpi-4.19.y$ make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- modules_prepare -j8
```

# 第一个树莓派驱动

入门例子当然是留给HelloWorld啦！

创建工程目录：
```sh
# 创建目录，进入
philon@a55v:~$ mkdir -p drivers/00-hello && cd drivers/00-hello
# 创建驱动模块源码及Makefile
philon@a55v:~/drivers/00-hello$ touch hello.c Makefile
```

hello.c
```c
#include <linux/init.h>
#include <linux/module.h>

static int __init hello_init(void)
{
  printk("hello kernel\n");
  return 0;
}
module_init(hello_init);

static void __exit hello_exit(void)
{
  printk("bye kernel\n");
}
module_exit(hello_exit);

MODULE_LICENSE("GPL v2"); // 开源许可证
MODULE_DESCRIPTION("hello module for RPi 3B+"); // 模块描述
MODULE_ALIAS("Hello"); // 模块别名
MODULE_AUTHOR("Philon"); // 模块作者
```

Makefile (为了便于内核模块构建，最好还是用make)
```Makefile
# 模块驱动，必须以obj-m=xxx形式编写
obj-m = hello.o

KDIR = ../linux-rpi-4.19.y
CROSS = ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-

all:
	$(MAKE) -C $(KDIR) M=$(PWD) $(CROSS) modules

clean:
	$(MAKE) -C $(KDIR) M=`pwd` $(CROSS) clean
````

```sh
# 编译内核
philon@a55v:~/drivers/00-hello$ make
# 将内核模块ko远程复制到开发板
philon@a55v:~/drivers/00-hello$ scp hello.ko rpi.local:/home/philon/modules/

# 开发板加载hello模块
philon@rpi:~/modules $ sudo insmod hello.ko
# 卸载内核
philon@rpi:~/modules $ sudo rmmod hello
# 查看内核打印信息
philon@rpi:~/modules $ dmesg | tail -5
[ 1176.602268] Under-voltage detected! (0x00050005)
[ 1182.842326] Voltage normalised (0x00000000)
[ 4731.671023] hello: no symbol version for module_layout
[ 4731.671042] hello: loading out-of-tree module taints kernel.
[ 4731.672307] hello kernel  👈看到没，就是它了！
[ 5010.908453] bye kernel
```
