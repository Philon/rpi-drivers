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