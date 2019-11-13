#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of_device.h>

#include "led.h"

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Philon | https://ixx.life");

// 当总线匹配到设备加载时调用该函数
static int led_probe(struct platform_device* dev) {
  printk("led probe\n");
  return 0;
}

// 当总线匹配到设备卸载时调用该函数
static int led_remove(struct platform_device* dev) {
  printk("led removed\n");
  return 0;
}

static const struct of_device_id of_leds_id[] = {
  { .compatible = "leda" },
  { .compatible = "ledb" },
  { .compatible = "ledc" },
};
// MODULE_DEVICE_TABLE(of, of_leds_id);

static struct platform_driver led_driver = {
  .probe = led_probe,
  .remove = led_remove,
  .driver = {
    .name = "my_led",
    .of_match_table = of_leds_id,
    .owner = THIS_MODULE,
  },

  .id_table = led_types,
};

// 【宏】向内核注册统一的led驱动
// 相当于同时定义了模块的入口和出口函数
// module_init(platform_driver_register)
// module_exit(platform_driver_unregister)
module_platform_driver(led_driver);