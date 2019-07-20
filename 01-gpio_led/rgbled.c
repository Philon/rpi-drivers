#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/io.h>
#include <asm/uaccess.h>

// BM2387B0(树莓派3B+)的外围起始地址是0x7F000000
// 根据BCM2835官方文档（P5和P90）显示，ARM的物理地址被映射到了0x20000000
// 但BCM2387的ARM外围物理地址映射为：0x3F000000
// GPIO偏移量0x200000，因此GPIO的ARM物理地址为0x3F200000
#define BCM2837_GPIO_BASE             0x3F200000
#define BCM2837_GPIO_FSEL0_OFFSET     0x0   // GPIO复用功能选择寄存器0
#define BCM2837_GPIO_SET0_OFFSET      0x1C  // GPIO置位寄存器0
#define BCM2837_GPIO_CLR0_OFFSET      0x28  // GPIO清零寄存器0

static void* gpio = 0;

static int __init rgbled_init(void)
{
  gpio = ioremap(BCM2837_GPIO_BASE, 0xB0);

  int val = ioread32(gpio + BCM2837_GPIO_FSEL0_OFFSET);
  val &= ~(7 << 6);
  val |= 1 << 6;

  iowrite32(val, gpio);
  iowrite32(1 << 2, gpio + BCM2837_GPIO_SET0_OFFSET);

  return 0;
}
module_init(rgbled_init);

static void __exit rgbled_exit(void)
{
  iowrite32(1 << 2, gpio + BCM2837_GPIO_CLR0_OFFSET);

}
module_exit(rgbled_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Philon | ixx.life");