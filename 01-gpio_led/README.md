# 树莓派驱动开发实战01：GPIO驱动之LED

本文源码：https://github.com/philon/rpi-drivers/tree/master/01-gpio-led

GPIO可以说是驱动中最最最简单的部分了，但我上网查了下，绝大部分所谓《树莓派GPIO驱动》的教程全是python、shell等编程，或者调用第三方库，根本不涉及任何ARM底层、Linux内核相关的知识。显然，这根本不是什么驱动实现，只是调用了一两个别人实现好的库函数而已，跟着那种文章走一遍，你只知道怎么用，永远不知道为什么。

所以本文是希望从零开始，在Linux内核下实现一个真正的gpio-led驱动程序，初步体验一下Linux内核模块的开发思想，知其然，知其所以然。

## GPIO基础

General-purpose input/output(通用输入/输出)，其引脚可由软件控制，选择输入、输出、中断、时钟、片选等不同的功能模式。以树莓派为例，我们可以通过[pinout官网](https://pinout.xyz)查看板子预留的40pinGPIO分别是做什么的。

![FD10F639-BBC1-4AB8-938C-7C69F3D005B4.png](https://i.loli.net/2019/07/20/5d32e43bd2d9783386.png)


如上图，GPIO0-1、GPIO2-3脚，除了常规的输入/输出，还可作为I²C接口，GPIO14-15脚，可另作为TTL串口。

总之，GPIO平时就是个普通IO口，仅作为开关用，但开关只是为了掩人耳目，背后的复用功能才是它的真正职业。

## 三色LED电路

弄懂了GPIO原理，那就来实际操作一把，准备点亮LED灯吧！

先来看看原理图，为了区分三色灯不同颜色的LED，我特别用红绿蓝接入对应的RGB三个灯，黑线表示GND。

![0E846F15-1316-471A-8EB6-37AAC4B6E379.png](https://i.loli.net/2019/07/20/5d32ef5346d9193879.png)

如图所示，三色灯的R、G、B正极分别接到树莓派GPIO的2、3、4脚，灯的公共负极随便接一个GND脚。因此，想要点亮其中一个灯，对应GPIO脚输出高电平即可，是不是很简单呐！

![C181BD91-B00A-4781-AED7-F40932D97C81.png](https://i.loli.net/2019/07/20/5d32f0f3ee4d365881.png)

## BCM2837寄存器分配

基于上述，要点亮LED只需要做一件事——GPIO输出高电平。如果通过程序让GPIO口输出高电平呢？

GPIO的控制其实是通过对应的CPU寄存器来实现的。在ARM架构的SoC中，所有的外围资源(寄存器)其实都是被映射到内存当中的，所以我们要读写寄存器，只需访问它映射到的内存地址即可。

那么问题来了，为什么不直接读写CPU的寄存器呢？因为现代的嵌入式系统往往都标配内存模块，处理器也带有MMU，所以其内部寄存器也就交由MMU来管理。

综上，我们现在要找出树莓派3B+这款芯片——BCM2837B0的GPIO物理内存地址。

*这里不得不吐槽一下，我先是跑到[树莓派3B+官方网址](https://www.raspberrypi.org/documentation/hardware/raspberrypi/bcm2837b0/README.md)去找芯片资料，得知BCM2837其实就是BCM2836的主频升级版；我又去看BCM2836的资料，得知这只不过是BCM2835从32位到64位的升级版；我又去看BCM2835的芯片资料，然而里面说的内存映射地址根本就是错的......*

要确定BCM2837B0的内存映射需要参考两个地方：
1. [BCM2835 Datasheet](https://www.raspberrypi.org/documentation/hardware/raspberrypi/bcm2835/BCM2835-ARM-Peripherals.pdf)，但要留意，里面坑很多，且并不完全适用于树莓派3B。
2. [国外热心网友的代码](https://github.com/Filkolev/LED-basic-driver)，仅包含GPIO驱动，没有太多细节

官方文档在第6页和第90页有这样一句话和这样几张表：

> Physical addresses range from 0x20000000 to 0x20FFFFFF for peripherals. The bus addresses for peripherals are set up to map onto the peripheral bus address range starting at 0x7E000000. Thus a peripheral advertised here at bus address 0x7Ennnnnn is available at physical address 0x20nnnnnn.

![BCM2835 GPIO 总线地址分配](https://i.loli.net/2019/07/20/5d3322e21519c31578.png)

![GPIO 复用功能选择](https://i.loli.net/2019/07/20/5d3323337992b55401.png)

我就直说吧，共6个关键要素：
- **外围总线地址0x7E000000映射到ARM物理内存地址0x20000000，故GPIO加上偏移，物理地址为0x20200000**
- **GPIO操作需要先通过GPFSEL选择复用功能，再通过GPSET/GPCLR对指定位拉高/拉低**
- **BMC2835共54个GPIO，分为两组BANK，第一组[0:31]，第二组[32:53]**
- **GPFSEL寄存器每3位表示一个GPIO的复用功能，因此一个寄存器可容纳10个GPIO，共6个GPFSEL**
- **GPSET/GPCLR寄存器每1位表示一个GPIO的状态1/0，因此一个寄存器可容纳32个GPIO，共2个GPSET/GPCLR**
- **⚠️国外热心网友指出：树莓派3B+的GPIO物理内存地址被映射到了0x3F200000！**

好了，现在结合电路图可推导出思路：
1. R、G、B分别对应GPIO2、3、4，需要操作的寄存器为GPFSEL0/GPSET0/GPCLR0
2. 要把三个脚全部设为“输出模式”，需要将GPFSEL0的第6、9、12都置为001
3. 要控制三个脚的输出状态，需要将GPSET0/GPCLR0的第2、3、4脚置1

## 先点亮红灯

现在开始写Linux驱动模块，先不着急完整实现，这一步只是把其中的红灯点亮，为此我甚至把绿蓝线给拔了！

代码实现非常简单，就是在加载驱动时红灯亮，卸载驱动时红灯灭。
```c
#include <linux/init.h>
#include <linux/module.h>
#include <asm/io.h>

#define BCM2837_GPIO_BASE             0x3F200000
#define BCM2837_GPIO_FSEL0_OFFSET     0x0   // GPIO功能选择寄存器0
#define BCM2837_GPIO_SET0_OFFSET      0x1C  // GPIO置位寄存器0
#define BCM2837_GPIO_CLR0_OFFSET      0x28  // GPIO清零寄存器0

static void* gpio = 0;

static int __init rgbled_init(void)
{
  // 获取GPIO对应的Linux虚拟内存地址
  gpio = ioremap(BCM2837_GPIO_BASE, 0xB0);

  // 将GPIO bit2设置为“输出模式”
  int val = ioread32(gpio + BCM2837_GPIO_FSEL0_OFFSET);
  val &= ~(7 << 6);
  val |= 1 << 6;

  // GPIO bit2 输出1
  iowrite32(val, gpio);
  iowrite32(1 << 2, gpio + BCM2837_GPIO_SET0_OFFSET);

  return 0;
}
module_init(rgbled_init);

static void __exit rgbled_exit(void)
{
  // GPIO输出0
  iowrite32(1 << 2, gpio + BCM2837_GPIO_CLR0_OFFSET);
  iounmap(gpio);

}
module_exit(rgbled_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Philon | ixx.life");
```

这段代码太简单了，以至于我觉得完全不需要解释，直接看效果吧。

![红灯点亮效果](https://i.loli.net/2019/07/20/5d3331f96425544265.gif)

## 再点亮全部

其实点亮红灯后，绿蓝灯无非是改改地址而已，没什么难度。本文的目的是学习Linux驱动，点亮LED不过是驱动开发的感性认识，所以下面几点才是本文的核心：

1. Linux内核模块之——字符设备
2. 如何动态申请主从设备号
3. 什么是Linux一切皆文件思想

```c
ssize_t rgbled_read(struct file* filp, char __user* buf, size_t len, loff_t* off)
{
  // todo: 通过读文件获取LED状态
}
ssize_t rgbled_write(struct file* filp, const char __user* buf, size_t len, loff_t* off)
{
  // todo: 通过写文件控制LED状态
}
long rgbled_ioctl(struct file* filp, unsigned int cmd, unsigned long arg)
{
  // todo: 通过ioctl函数控制每个灯的状态
}

const struct file_operations fops = {
  .owner = THIS_MODULE,
  .read = rgbled_read,
  .write = rgbled_write,
  .unlocked_ioctl = rgbled_ioctl,
};
```

```sh
# 加载驱动 & 查看模块的主从设备号
philon@rpi:~/modules$ sudo insmod rgbled.ko
philon@rpi:~/modules$ dmesg 
...
[  106.818009] rgbled: no symbol version for module_layout
[  106.818028] rgbled: loading out-of-tree module taints kernel.
[  106.820307] rgbled device major&minor is [240:0]

# 根据动态分配的设备号创建设备节点
philon@rpi:~/modules$ sudo mknod /dev/rgbled c 240 0
```