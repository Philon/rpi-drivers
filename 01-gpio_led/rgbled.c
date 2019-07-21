#include <linux/init.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <asm/io.h>

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Philon | https://ixx.life");

#define BCM2837_GPIO_BASE             0x3F200000
#define BCM2837_GPIO_FSEL0_OFFSET     0x0   // GPIO功能选择寄存器0
#define BCM2837_GPIO_SET0_OFFSET      0x1C  // GPIO置位寄存器0
#define BCM2837_GPIO_CLR0_OFFSET      0x28  // GPIO清零寄存器0

#define LED_RED_PIN     2
#define LED_GREEN_PIN   3
#define LED_BLUE_PIN    4

static void* gpio = 0;
static char color[32] = {0};

void gpioctl(int pin, int stat)
{
  void* reg = gpio + (stat ? BCM2837_GPIO_SET0_OFFSET : BCM2837_GPIO_CLR0_OFFSET);
  iowrite32(1 << pin, reg);
}

// 向用户层返回当前设备颜色值
ssize_t rgbled_read(struct file* filp, char __user* buf, size_t len, loff_t* off)
{
  char data[32] = {0};

  len = strlen(color) + 1;
  sprintf(data, "%s\n", color);
  if (copy_to_user(buf, data, len) < 0) {
    return -EIO;
  }

  return len;
}

// 用户层通过颜色名称来控制设备
ssize_t rgbled_write(struct file* filp, const char __user* buf, size_t len, loff_t* off)
{
  const char* colors[] = {"white", "black", "red", "green", "blue", "yellow", "cyan", "purple"};
  const int pins[][3] = {{1,1,1}, {0,0,0}, {1,0,0}, {0,1,0}, {0,0,1}, {1,1,0}, {0,1,1}, {1,0,1}};
  char data[32] = {0};
  int i = 0;

  if (copy_from_user(data, buf, len) < 0) {
    return -EIO;
  }

  for (i = 0; i < 8; i++) {
    if (!strncasecmp(colors[i], data, strlen(colors[i]))) {
      strcpy(color, data);
      gpioctl(LED_RED_PIN, pins[i][0]);
      gpioctl(LED_GREEN_PIN, pins[i][1]);
      gpioctl(LED_BLUE_PIN, pins[i][2]);
      return len;
    }
  }

  return -EINVAL;
}

// 用户层通过ioctl函数单独控制灯的状态
long rgbled_ioctl(struct file* filp, unsigned int cmd, unsigned long arg)
{
  switch (cmd) {
    case 'r':
      gpioctl(LED_RED_PIN, arg);
      break;
    case 'g':
      gpioctl(LED_GREEN_PIN, arg);
      break;
    case 'b':
      gpioctl(LED_BLUE_PIN, arg);
      break;
    default:
      return -ENODEV;
  }

  return 0;
}

static const struct file_operations fops = {
  .owner = THIS_MODULE,
  .read = rgbled_read,
  .write = rgbled_write,
  .unlocked_ioctl = rgbled_ioctl,
};

static dev_t devno = 0;   // 设备编号
static struct cdev cdev;  // 字符设备结构体

static int __init rgbled_init(void)
{
  // 映射GPIO物理内存到虚拟地址，并将其置为“输出模式”
  // 代码写得比较丑，解释以下：
  // 就是先把三个GPIO的“功能选择位”全部置000
  // 然后再将其置为001
  int val = ~((7 << (LED_RED_PIN*3)) | (7 << (LED_GREEN_PIN*3)) | (7 << LED_BLUE_PIN*3));
  gpio = ioremap(BCM2837_GPIO_BASE, 0xB0);
  val &= ioread32(gpio + BCM2837_GPIO_FSEL0_OFFSET);
  val |= (1 << (LED_RED_PIN*3)) | (1 << (LED_GREEN_PIN*3)) | (1 << (LED_BLUE_PIN*3));
  iowrite32(val, gpio);

  // 将该模块注册为一个字符设备，并动态分配设备号
  if (alloc_chrdev_region(&devno, 0, 1, "rgbled")) {
    printk(KERN_ERR"failed to register kernel module!\n");
    return -1;
  }
  cdev_init(&cdev, &fops);
  cdev_add(&cdev, devno, 1);

  printk(KERN_INFO"rgbled device major & minor is [%d:%d]\n", MAJOR(devno), MINOR(devno));

  return 0;
}
module_init(rgbled_init);

static void __exit rgbled_exit(void)
{
  // 取消gpio物理内存映射
  iounmap(gpio);

  // 释放字符设备
  cdev_del(&cdev);
  unregister_chrdev_region(devno, 1);

  printk(KERN_INFO"rgbled free\n");
}
module_exit(rgbled_exit);
