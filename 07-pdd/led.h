#ifndef RPI_DRIVER_PDD_LED_H
#define RPI_DRIVER_PDD_LED_H

// 自定义平台数据，给总线、驱动、设备用
struct led {
  int id;
  char* name;
}

#endif //!RPI_DRIVER_PDD_LED_H