#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Philon | https://ixx.life");

static const struct gpio ir = {
  .gpio = 18,
  .flags = GPIOF_IN,
  .label = "InfraredReceive",
};

static struct {
  int irq;
} infrared;

static irqreturn_t on_recv(int irq, void* dev) {
  printk(KERN_INFO "get irq\n");
  return IRQ_HANDLED;
}

static int __init infrared_init(void) {
  int rc = 0;

  if ((rc = gpio_request_one(ir.gpio, ir.flags, ir.label)) < 0) {
    printk(KERN_ERR "ERROR%d: can not request gpio%d\n", rc, ir.gpio);
    return rc;
  }

  infrared.irq = gpio_to_irq(ir.gpio);
  if ((rc = request_irq(infrared.irq, on_recv, IRQF_TRIGGER_RISING, "IR", NULL)) < 0) {
    printk(KERN_ERR "ERROR%d: can not request irq\n", infrared.irq);
    return rc;
  }
  return 0;
}
module_init(infrared_init);

static void __exit infrared_exit(void) {
  free_irq(infrared.irq, NULL);
  gpio_free(ir.gpio);
}
module_exit(infrared_exit);