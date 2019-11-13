#include <linux/module.h>
#include <linux/platform_device.h>

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Philon | https://ixx.life");

static void led_release(struct device* pdev) {
  printk("led release!\n");
}

static struct platform_device led_device = {
  .name = "type0_led",
  .dev = {
    .release = led_release,
  },
};

static int leddev_init(void) {
  platform_device_register(&led_device);
  return 0;
}
module_init(leddev_init);

static void leddev_exit(void) {
  platform_device_unregister(&led_device);
}
module_exit(leddev_exit);