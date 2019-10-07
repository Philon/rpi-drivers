#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/timer.h>
#include <linux/wait.h>

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Philon | https://ixx.life");

static struct {
  int gpio;
  int irq;
  wait_queue_head_t rwait;
  struct timer_list timer;
  u32     pulse;  // 脉冲上升沿持续时长
  u32     space;  // 脉冲下降沿持续时长
  size_t  count;  // 脉冲个数
  u32     data;   // 脉冲解码后的值
} ir;

#define is_head(p, s) (p > 8900 && p < 9100 && s > 4400 && s < 4600)
#define is_repeat(p, s) (p > 8900 && p < 9100 && s > 2150 && s < 2350)
#define is_bfalse(p, s) (p > 500 && p < 650 && s > 500 && s < 650)
#define is_btrue(p, s) (p > 500 && p < 650 && s > 1500 && s < 1750)

// 红外接收函数(即GPIO18的双边沿中断处理函数)
// 记录GPIO每次中断是“上升还是下降”，以及持续的时长
static irqreturn_t ir_rx(int irq, void* dev) {
  static ktime_t last = 0;
  u32 duration = (u32)ktime_to_us(ktime_get() - last);

  // ⚠️注意：1838红外头高低电平逻辑取反
  if (!gpio_get_value(ir.gpio)) {
    ir.space = duration;
  } else {
    // 切换下降沿时，脉冲只有高电平部分，所以不做处理
    ir.pulse = duration;
    goto irq_out;
  }

  if (is_head(ir.pulse, ir.space)) {
    ir.count = ir.data = 0;
  } else if (is_repeat(ir.pulse, ir.space)) {
    ir.count = 32;
  } else if (is_btrue(ir.pulse, ir.space)) {
    ir.data |= 1 << ir.count++;
  } else if (is_bfalse(ir.pulse, ir.space)) {
    ir.data |= 0 << ir.count++;
  } else {
    goto irq_out;
  }

  if (ir.count >= 32) {
    wake_up(&ir.rwait);
  }

irq_out:
  mod_timer(&ir.timer, jiffies + (HZ / 10));
  last = ktime_get();
  return IRQ_HANDLED;
}

// 定时清除红外协议帧的相关信息，便于接收下一帧
static void clear_flag(struct timer_list *timer) {
  ir.pulse = 0;
  ir.space = 0;
  ir.count = 0;
  ir.data = 0;
}

static ssize_t ir_read(struct file *filp, char __user *buf, size_t len, loff_t *off) {
  int rc = 0;

  if ((filp->f_flags & O_NONBLOCK) && ir.count < 32) {
    return -EAGAIN;
  } else {
    DECLARE_WAITQUEUE(wq, current);
    add_wait_queue(&ir.rwait, &wq);
    wait_event(ir.rwait, ir.count == 32);
    remove_wait_queue(&ir.rwait, &wq);
  }

  rc = copy_to_user(buf, &ir.data, sizeof(u32));
  if (rc < 0) {
    return rc;
  }

  ir.count = 0;
  *off += sizeof(u32);
  return sizeof(u32);
}

static const struct file_operations fops = {
  .owner = THIS_MODULE,
  .read = ir_read,
};

static struct miscdevice irdev = {
  .minor = MISC_DYNAMIC_MINOR,
  .name = "IR1838-NEC",
  .fops = &fops,
  .nodename = "ir0",
  .mode = 0744,
};

static int __init ir_init(void) {
  int rc = 0;

  // 初始化脉冲处理函数
  init_waitqueue_head(&ir.rwait);

  // 初始化定时器，用于断帧
  timer_setup(&ir.timer, clear_flag, 0);
  add_timer(&ir.timer);

  // 申请GPIO及其双边沿中断
  ir.gpio = 18;
  if ((rc = gpio_request_one(ir.gpio, GPIOF_IN, "IR")) < 0) {
    printk(KERN_ERR "ERROR%d: can not request gpio%d\n", rc, ir.gpio);
    return rc;
  }

  ir.irq = gpio_to_irq(ir.gpio);
  if ((rc = request_irq(ir.irq, ir_rx,
              IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
              "IR", NULL)) < 0) {
    printk(KERN_ERR "ERROR%d: can not request irq\n", ir.irq);
    return rc;
  }

  if ((rc = misc_register(&irdev)) < 0) {
    return rc;
  }

  return 0;
}
module_init(ir_init);

static void __exit ir_exit(void) {
  misc_deregister(&irdev);
  free_irq(ir.irq, NULL);
  gpio_free(ir.gpio);
  del_timer(&ir.timer);
}
module_exit(ir_exit);