#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h> // 混杂设备相关结构
#include <linux/gpio.h>       // 各种gpio的数据结构及函数
#include <linux/interrupt.h>  // 内核中断相关接口
#include <linux/delay.h>

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

// 定义三色LED的GPIO引脚
static const struct gpio leds[] = {
  { 2, GPIOF_OUT_INIT_HIGH, "LED_RED" },
  { 3, GPIOF_OUT_INIT_HIGH, "LED_GREEN" },
  { 4, GPIOF_OUT_INIT_HIGH, "LED_BLUE" },
};

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

// 定义本模块为混杂设备
static struct miscdevice dev = {
  .minor = 0,
  .name = "gpio_key", // 模块名
  .nodename = "key0", // 设备节点名 /dev/gpiokey0
  .fops = &fops,
  .mode = 0600
};


// 按键中断“顶半部”处理函数
static irqreturn_t on_key_press(int irq, void* dev)
{
  static int next = 0;

  printk(KERN_INFO "key0 press\n");
  gpio_set_value(leds[next].gpio, 0);
  next = next >= 2 ? 0 : next+1;
  gpio_set_value(leds[next].gpio, 1);

  return IRQ_HANDLED;
}

static int __init gpiokey_init(void)
{
  int rc = 0;

  // 向内核申请GPIO
  if ((rc = gpio_request_one(key.gpio, key.flags, key.label)) < 0
    || (rc = gpio_request_array(leds, 3)) < 0) {
      printk(KERN_ERR "ERROR%d: cannot request gpio\n", rc);
      return rc;
    }

  // 获取中断号
  key_irq = gpio_to_irq(key.gpio);
  if (key_irq < 0) {
    printk(KERN_ERR "can not get irq num.\n");
    return -EFAULT;
  }

  printk(KERN_ALERT "get irq = %d\n", key_irq);

  // 申请上升沿触发
  if (request_irq(key_irq, on_key_press, IRQF_TRIGGER_RISING, "onKeyPress", (void*)&dev) < 0) {
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
  free_irq(key_irq, NULL);
  gpio_free_array(leds, 3);
  gpio_free(key.gpio);
  misc_deregister(&dev);
}
module_exit(gpiokey_exit);