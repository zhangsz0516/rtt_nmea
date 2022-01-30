#ifndef __RIL_VDEV_H__
#define __RIL_VDEV_H__

#include <rtthread.h>
#include <rtdevice.h>

#define RIL_VDEV_CTRL_CMD_POWER_OFF          0xAA00
#define RIL_VDEV_CTRL_CMD_POWER_ON           0xAA01

int ril_vdevice_init(void);

#endif
