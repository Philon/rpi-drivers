# 树莓派驱动开发实战07:PDD与设备树

从《树莓派驱动开发实战》的第一篇至今，我都是在写独立的设备驱动，并没有触及到内核的高阶玩法。之后的驱动开发更多会涉及USB、I2C、UART之类的总线设备，也为了更好地理解Linux驱动架构，我觉得不妨先学习**平台驱动设备**模型。

平台-驱动-设备(Platform-Driver-Device)，即以面向对象的角度，将那些具有共性的代码抽象到一个驱动里，并通过平台总线匹配到对应的设备当中。不过在详细介绍PDD模型之前，需要先了解驱动-总线-设备模型。

## 驱动-总线-设备模型

为解决同类设备驱动程序的复用性，Linux2.6之后引入了全新驱动注册管理机制：**驱动、总线、设备**。

- 驱动程序：负责实现设备共有的业务逻辑
- 设备程序：负责描述不同设备的硬件资源部分
- 总线程序：负责实现总线的业务逻辑，设备与驱动的匹配规则

举例，USB总线上，挂着多个USB驱动，当总线监听到有设备插入时，总线会识别USB设备类型，并根据找到对应的驱动程序，驱动程序又根据设备描述将其注册到内核。

内核提供了相应的`bus、device、driver、class`等最为底层的API和数据结构，一般情况下是用不到的。USB、I2C、SPI、uart/tty等物理总线都基于这些API封装了对应的，如`usb_bus、usb_device、usb_driver、tty_device、tty_driver`等接口，日常的设备驱动开发大多基于这一层提供的相关接口。

```sh
philon@rpi:~ $ ls /sys/bus/
amba         container     genpd  i2c              media     mmc_rpmb  scsi  usb
clockevents  cpu           gpio   iscsi_flashnode  mipi-dsi  nvmem     sdio  workqueue
clocksource  event_source  hid    mdio_bus         mmc       platform  spi
```

通过`/sys/bus`目录可以看到系统当前存在的总线。👆注意看倒数第二个，`platform`平台总线出现了！！

## 平台总线、平台驱动、平台设备

**platform是一种虚拟总线**。与`usb_bus tty_bus spi_bus`等物理总线平级。

在编写字符设备`cdev`时需要关心主、次设备号，还要用mknod命令创建对应的设备节点。但不是每个设备都需要固定的设备号的，内核提供了一个叫`misc`混杂设备的概念，不用关心设备号还能自动创建设备节点。统一主设备号固定为10，本质上就是基于cdev再封装了一层。

同理，硬件物理总线：USB、I2C、SPI、UART等，它的时序/电平/通信机制都不一样，在内核中会有对应的`bus_type`，对应的设备归口对应的总线和驱动。但也有些设备并不挂到总线上，如：led、蜂鸣器、摄像头等，为了符合驱动-总线-设备的架构，就把它们统一归口到Platform这根“假”总线下。

简单理解，平台设备的角色有点像混杂设备的架构升级版。实际研发中，很多开发板的外围模块都会划归给platform管理。

这种做法的好处很明显，假设一个路由器有8个LED指示灯，那我只需要编写一个`platform_led_driver`驱动和8个`platform_led_device`描述即可，剩下的事情交给内核自带的`platform_bus`去匹配就好。

如果你愿意，可以浏览下内核目录`arch/arm/mach-xxx`，非常多对吧。其实这些目录大多是平台设备描述，用于适配各大厂商不同型号的处理器或开发板。这玩意儿后来招致Linus大怒——简直就是一坨屎！毕竟这是各家针对硬件细节的BSP(板级支持包)，你把一份份电路板“说明书”提交至通用的内核代码里，这是人干的事儿？

因为这个典故，才有了后来的`device-tree`，之后详细介绍，现在先巩固PDD模型的知识。

## 最简单的PDD实例

以下只是以led为例说明platform相关接口用法，并未真正实现led驱动。

led_driver.c

```c
#include <linux/module.h>
#include <linux/platform_device.h>

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Philon | https://ixx.life");

// 当总线匹配到设备时调用该函数
static int led_probe(struct platform_device* dev) {
  printk("led %s probe\n", dev->name);
  // todo: 字符设备注册、gpio申请之类的
  return 0;
}

// 当总线匹配到设备卸载时调用该函数
static int led_remove(struct platform_device* dev) {
  printk("led %s removed\n", dev->name);
  // todo: 其他资源释放
  return 0;
}

// 平台驱动描述
static struct platform_driver led_driver = {
  .probe = led_probe,
  .remove = led_remove,
  .driver = {
    .name = "my_led", // 👈务必注意，platform是以name比对来匹配的
    .owner = THIS_MODULE,
  },
};

// 【宏】向内核注册统一的led驱动
// 相当于同时定义了模块的入口和出口函数
// module_init(platform_driver_register)
// module_exit(platform_driver_unregister)
module_platform_driver(led_driver);
```

led_device.c

```c
#include <linux/module.h>
#include <linux/platform_device.h>

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Philon | https://ixx.life");

// 最好实现该接口，否则在设备释放的时候内核会报错
static void led_release(struct device* pdev) {
  printk("led release!\n");
}

// 平台设备描述
static struct platform_device led_device = {
  .name = "my_led", // 👈要确保与led_driver的定义一致，否则匹配不上
  .dev = {
    .release = led_release,
  },
};

// 注册平台设备
static int leddev_init(void) {
  platform_device_register(&led_device);
  return 0;
}
module_init(leddev_init);

static void leddev_exit(void) {
  platform_device_unregister(&led_device);
}
module_exit(leddev_exit);
```

首先，加载led_driver.ko模块后，可以在平台总线目录下看到`my_led`驱动了。然后，加载led_device.ko模块后，同样可以在平台总线设备里查看到`my_led.0`的设备。
```sh
# 平台总线里查看my_led驱动
philon@rpi:~/modules $ sudo insmod led_driver.ko 
philon@rpi:~/modules $ ls /sys/bus/platform/drivers/my_led/
bind  module  uevent  unbind

# 平台总线里查看my_led设备
philon@rpi:~/modules $ sudo insmod led_device.ko      
philon@rpi:~/modules $ ls /sys/bus/platform/devices/my_led.0/
driver  driver_override  modalias  power  subsystem  uevent

# 看一下内核打印信息
philon@rpi:~/modules $ dmesg
[225668.547712] led my_led probe
[225687.213336] led my_led removed
[225687.213448] led release!
```

⚠️注意：不必操心`driver/device`模块的加载顺序，谁先谁后都一样，platform_bus会料理好一切。

## 设备树