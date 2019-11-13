#ifndef RPI_DRIVER_PDD_LED_H
#define RPI_DRIVER_PDD_LED_H

#include <linux/mod_devicetable.h>

struct platform_device_id led_types[] = {
  { .name = "type0_led" },
  { .name = "type1_led" },
  {},
};

#endif //!RPI_DRIVER_PDD_LED_H