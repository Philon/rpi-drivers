#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Philon | https://ixx.life");

#define BCM2837_GPIO_BASE             0x3F200000
#define GPIO_KEY_PIN                  25

static ssize_t gpiokey_read(struct file *filp, char __user *buf, size_t len, loff_t *off)
{
  return 0;
}
// static long (*unlocked_ioctl) (struct file *, unsigned int, unsigned long);
// static int gpiokey_open(struct inode *, struct file *);
// static int gpiokey_close(struct inode *, struct file *)

static const struct file_operations fops = {
  .owner = THIS_MODULE,
  .read = gpiokey_read,
  // .write = rgbled_write,
  // .unlocked_ioctl = rgbled_ioctl,
};

static struct miscdevice dev = {
  .minor = 0,
  .name = "gpio_key",      // 模块名
  .nodename = "key0", // 设备节点名 /dev/gpiokey0
  .fops = &fops,
  .mode = 0600
};

static irqreturn_t on_key0_press(int irq, void* dev)
{
  printk(KERN_INFO "key0 press\n");
  return IRQ_HANDLED;
}

static struct {
  int irq;
  int pin;
} key0 = {0, BCM2837_GPIO_BASE + GPIO_KEY_PIN};

static int __init gpiokey_init(void)
{
  // 向内核申请占用GPIO
  if (gpio_request(key0.pin, "Key0") < 0) {
    printk(KERN_ERR "can not request gpio\n");
    return -EFAULT;
  }

  // 获取中断号
  key0.irq = gpio_to_irq(key0.pin);
  if (key0.irq < 0) {
    printk(KERN_ERR "can not get irq num.\n");
    return -EFAULT;
  }

  // 申请上升沿触发
  if (request_irq(key0.irq, on_key0_press, IRQF_TRIGGER_RISING, "onKeyPress", (void*)&dev) < 0) {
    printk(KERN_ERR "can not request irq\n");
    return -EFAULT;
  }

  // 使用混杂设备注册，等同于完成内核模块的所有注册工作
  // 并在用户层自动创建和删除设备节点
  // alloc_chrdev_region + cdev_init + cdev_add
  misc_register(&dev);

  return 0;
}
module_init(gpiokey_init);

static void __exit gpiokey_exit(void)
{
  free_irq(key0.irq, (void*)&dev);
  misc_deregister(&dev);
}
module_exit(gpiokey_exit);