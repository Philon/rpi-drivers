# 树莓派驱动开发实战02：GPIO驱动之按键中断

在上一篇中主要学习了GPIO原理及Linux字符设备，其过程大致是这样子的：

```
电路图“看引脚” --> 手册“看物理地址” --> 寄存器手册“看GPIO逻辑控制” --> 复用功能 --> 地址操作
```

可以看到，整个过程是非常繁琐的，驱动程序必须精确到处理器的每一根引脚的状态，如果所有驱动都这么写估计当场就跪了。因此，本文除了学习GPIO中断原理之外，更重要是掌握以下知识：

- 混杂设备机制
- GPIO内核结构调用
- Linux内核中断与时钟

先来看个接线图，为了更好地展示中断，“继承”了上一篇文章的三色LED接线，预期要实现是“每按一次键改变一种颜色”。

![接线图](https://i.loli.net/2019/08/04/7EYIMusdiBWhv5Q.png)

从下图中可以看到，按键Key的两个脚接到了树莓派的`GPIO17`和`3V3`上，换句话说，就是用GPIO17接收上升沿中断信号。而LED的电路及控制原理保持之前的不变。

![电路图](https://i.loli.net/2019/08/04/JlXWK7bcs4Igjxv.png)

## GPIO/IRQ基础

中断的实现分为两个部分：**GPIO中断功能选择**和**Linux内核中断申请**。因为电路上的中断是由上升沿触发，故将GPIO-17设为“输入”模式，并向内核获取对应的“中断号”，注册对应的中断处理函数即可。

**1. GPIO资源申请**

一般而言，芯片厂商都会将ARM的外设寄存器地址封装为固定的接口，理想情况下是支持内核的标准接口的。尽量使用内核接口编写驱动程序，一能保证底层代码的质量，二能提高代码的移植性。

之前写三色LED驱动时，我们需要确定物理内存地址、功能选择寄存器、数据位、控制地址、状态位等等，极其繁琐。其实这些功能都被封装到几个简单的API里了。

```c
#include <linux/gpio.h>

struct gpio {
	unsigned	gpio;       // GPIO编号
	unsigned long	flags;  // GPIO复用功能配置
	const char	*label;   // GPIO标签名
};

// 单个GPIO资源申请/释放
int gpio_request_one(unsigned gpio, unsigned long flags, const char *label);
void gpio_free(unsigned gpio);

// 多个GPIO资源申请/释放
int gpio_request_array(const struct gpio *array, size_t num);
void gpio_free_array(const struct gpio *array, size_t num);

// GPIO状态读写
int gpio_get_value(unsigned gpio);
void gpio_set_value(unsigned gpio, int value);
```

此外，采用request函数申请资源时，对于那些以“独占”方式申请的GPIO可以起到保护作用，不过前提是每个驱动程序都按规矩使用内核API，如果你偏要直接通过寄存器地址去访问，那“冲突”的情况就得你自己去搞定。

**2. IRQ中断申请**

理论上来说，每个GPIO都可以向内核注册为

*PS: 《BCM2835官方手册第7章》专门说明中断方式，分为GPU中断和外设中断，并可通过寄存器设置将GPIO设为“上升沿中断”模式，由于本文更侧重Linux驱动编程，具体的芯片及电路原理我没有深究。*

## 最简单的GPIO中断
```c
#include <linux/module.h>
#include <linux/gpio.h>       // 各种gpio的数据结构及函数
#include <linux/interrupt.h>  // 内核中断相关接口

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Philon | https://ixx.life");

// 稍后由内核分配的按键中断号
static unsigned int key_irq = 0;

// 定义按键的GPIO引脚功能
static const struct gpio key = {
  .gpio = 17,         // 引脚号为BCM - 20
  .flags = GPIOF_IN,  // 功能复用为输入
  .label = "Key0"     // 标示为Key0
};

// 按键中断“顶半部”处理函数
static irqreturn_t on_key_press(int irq, void* dev)
{
  printk(KERN_INFO "key0 press\n");
  return IRQ_HANDLED;
}

static int __init gpiokey_init(void)
{
  int rc = 0;

  // 向内核申请GPIO
  if ((rc = gpio_request_one(key.gpio, key.flags, key.label)) < 0) {
      printk(KERN_ERR "ERROR%d: cannot request gpio\n", rc);
      return rc;
    }

  // 获取中断号
  key_irq = gpio_to_irq(key.gpio);
  if (key_irq < 0) {
    printk(KERN_ERR "ERROR%d:cannot get irq num.\n", key_irq);
    return key_irq;
  }

  // 申请上升沿触发中断
  if (request_irq(key_irq, on_key_press, IRQF_TRIGGER_RISING, "onKeyPress", NULL) < 0) {
    printk(KERN_ERR "cannot request irq\n");
    return -EFAULT;
  }

  return 0;
}
module_init(gpiokey_init);

static void __exit gpiokey_exit(void)
{
  // 释放中断号及GPIO
  free_irq(key_irq, NULL);
  gpio_free(key.gpio);
}
module_exit(gpiokey_exit);
```