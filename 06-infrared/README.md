# 树莓派驱动开发实战06：红外遥控

```sh
[  203.718032] -----------------_________-_-_-_-_-_-_-_-_-___-___-___-___-___-___-___-___-_-_-_-___-_-_-_-_-___-___-___-_-___-___-___-___-_
[  207.647870] -----------------_________-_-_-_-_-_-_-_-_-___-___-___-___-___-___-___-___-_-_-___-___-___-_-_-_-___-___-_-_-_-___-___-___-_
[  209.927802] -----------------_________-_-_-_-_-_-_-_-_-___-___-___-___-___-___-___-___-_-___-_-___-___-_-___-_-___-_-___-_-_-___-_-___-_
[  214.557679] -----------------_________-_-_-_-_-_-_-_-_-___-___-___-___-___-___-___-___-_-___-_-_-_-_-___-_-___-_-___-___-___-___-_-___-_
[  216.917629] -----------------_________-_-_-_-_-_-_-_-_-___-___-___-___-___-___-___-___-_-___-_-_-___-_-___-_-___-_-___-___-_-___-_-___-_
[  219.457571] -----------------_________-_-_-_-_-_-_-_-_-___-___-___-___-___-___-___-___-_-___-_-___-_-_-___-_-___-_-___-_-___-___-_-___-_
```

```c
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/timer.h>
#include <linux/vmalloc.h>

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Philon | https://ixx.life");

static struct {
  int gpio;
  int irq;
  struct timer_list timer;
} ir;

static struct {
  bool direction;
  s64  keep;
} frame[127] = {0};
static int count = 0;

// 红外接收函数(即GPIO18的双边沿中断处理函数)
// 记录GPIO每次中断是“上升还是下降”，以及持续的时长
static irqreturn_t ir_rx(int irq, void* dev) {
  s64 now = ktime_to_us(ktime_get());

  frame[count].direction = !gpio_get_value(ir.gpio);
  frame[count].keep = now;
  if (count > 0) {
    frame[count-1].keep = now - frame[count-1].keep;
  }
  count++;

  mod_timer(&ir.timer, jiffies + (HZ / 10));

  return IRQ_HANDLED;
}

// 定时清除红外协议帧的相关信息，便于接收下一帧
// 清除之前打印当前帧的波形
void clear_flag(struct timer_list *timer) {
  char out[2048] = {'\0'};
  char* p = out;
  int i = 0;

  // 打印波形
  frame[count-1].keep = 500;
  for (i = 0; i < count; i++) {
    char c = frame[i].direction ? '-' : '_';
    int n = (int)frame[i].keep;
    n /= 500;
    while (n--) {
      *p++ = c;
    }
  }
  *p = '\0';
  printk("%s\n", out);

  memset(frame, 0, sizeof(frame));
  count = 0;
}

// 申请GPIO18为输入模式，并注册“上升沿/下降沿”的中断
static int __init infrared_init(void) {
  int rc = 0;

  timer_setup(&ir.timer, clear_flag, 0);
  add_timer(&ir.timer);

  ir.gpio = 18;
  if ((rc = gpio_request_one(ir.gpio, GPIOF_IN, "IR")) < 0) {
    printk(KERN_ERR "ERROR%d: can not request gpio%d\n", rc, ir.gpio);
    return rc;
  }

  ir.irq = gpio_to_irq(ir.gpio);
  if ((rc = request_irq(ir.irq, ir_rx, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "IR", NULL)) < 0) {
    printk(KERN_ERR "ERROR%d: can not request irq\n", ir.irq);
    return rc;
  }
  return 0;
}
module_init(infrared_init);

static void __exit infrared_exit(void) {
  free_irq(ir.irq, NULL);
  gpio_free(ir.gpio);
  del_timer(&ir.timer);
}
module_exit(infrared_exit);
```