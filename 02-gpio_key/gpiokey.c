#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h> // 混杂设备相关结构
#include <linux/gpio.h>       // 各种gpio的数据结构及函数
#include <linux/interrupt.h>  // 内核中断相关接口
#include <linux/workqueue.h>
#include <linux/timer.h>

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Philon | https://ixx.life");

// 定义按键的GPIO引脚
static const struct gpio key = {
  .gpio = 17,         // 引脚号为BCM - 17
  .flags = GPIOF_IN,  // 功能复用为输入
  .label = "Key0"     // 标示为Key0
};

// 定义三色LED的GPIO引脚
static const struct gpio leds[] = {
  { 2, GPIOF_OUT_INIT_HIGH, "LED_RED" },
  { 3, GPIOF_OUT_INIT_HIGH, "LED_GREEN" },
  { 4, GPIOF_OUT_INIT_HIGH, "LED_BLUE" },
};

static unsigned int keyirq = 0;     // GPIO按键中断号
static struct work_struct keywork;  // 按键工作队列
static struct timer_list timer;     // 定时器作为中断延时

// 按键中断“顶半部”处理函数，启用工作队列
static irqreturn_t on_key_press(int irq, void* dev)
{
  schedule_work(&keywork);
  return IRQ_HANDLED;
}

// 按键中断“底半部”工作队列，启动一个50ms的延时定时器
void start_timer(struct work_struct *work)
{
  mod_timer(&timer, jiffies + (HZ/20));
}

// 按键防抖定时器，及处理函数
void on_delay_50ms(struct timer_list *timer)
{
  static int i = 0;
  if (gpio_get_value(key.gpio)) {
    gpio_set_value(leds[i].gpio, 0);
    i = ++i == 3 ? 0 : i;
    gpio_set_value(leds[i].gpio, 1);
  }
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
  keyirq = gpio_to_irq(key.gpio);
  if (keyirq < 0) {
    printk(KERN_ERR "can not get irq num.\n");
    return -EFAULT;
  }

  // 申请上升沿触发
  if (request_irq(keyirq, on_key_press, IRQF_TRIGGER_RISING, "onKeyPress", NULL) < 0) {
    printk(KERN_ERR "can not request irq\n");
    return -EFAULT;
  }

  // 初始化按键中断底半部(工作队列)
  INIT_WORK(&keywork, start_timer);

  // 初始化定时器
  timer_setup(&timer, on_delay_50ms, 0);
  add_timer(&timer);

  return 0;
}
module_init(gpiokey_init);

static void __exit gpiokey_exit(void)
{
  free_irq(keyirq, NULL);
  gpio_free_array(leds, 3);
  gpio_free(key.gpio);
  del_timer(&timer);
}
module_exit(gpiokey_exit);