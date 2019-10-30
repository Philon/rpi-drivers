#include <linux/module.h>
#include <linux/platform_device.h>

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Philon | https://ixx.life");

// static struct resource leds[] = {
//   [0] = {
//     .start = 0x00,
//     .end = 0x0F,
//     .name = "led_gpio",
//     .flags = IORESOURCE_IO,
//   },
//   [1] = {
//     .start = 0x10,
//     .end = 0x0F,
//     .name = "irq",
//     .flags = IORESOURCE_IRQ,
//   }
// };

static struct platform_device led_device = {
  .name = "my_led",
  .id = -1,
  .dev = {
    .platform_data = NULL,
    // .release = led_release,
  },
  // .resource = leds,
  // .num_resources = ARRAY_SIZE(leds)
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