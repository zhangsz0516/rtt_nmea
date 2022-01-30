#ifndef __RIL_APP_H__
#define __RIL_APP_H__

#include "ril_vdev.h"

rt_err_t ril_app_init(void);
rt_err_t ril_vdev_power_on(void);
rt_err_t ril_vdev_power_off(void);
int ril_at_device_init(void);

#endif
