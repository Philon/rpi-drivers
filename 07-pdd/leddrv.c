#include <linux/module.h>
#include <linux/platform_device.h>

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Philon | https://ixx.life");

// 当总线匹配到设备加载时调用该函数
static int led_probe(struct platform_device* dev) {
  printk("led %s probe\n", dev->name);
  return 0;
}

// 当总线匹配到设备卸载时调用该函数
static int led_remove(struct platform_device* dev) {
  printk("led %s removed\n", dev->name);
  return 0;
}

static struct platform_driver led_driver = {
  .probe = led_probe,
  .remove = led_remove,
  .driver = {
    .name = "my_led",
    .owner = THIS_MODULE,
  },
};

// 【宏】向内核注册统一的led驱动
// 相当于同时定义了模块的入口和出口函数
// module_init(platform_driver_register)
// module_exit(platform_driver_unregister)
module_platform_driver(led_driver);