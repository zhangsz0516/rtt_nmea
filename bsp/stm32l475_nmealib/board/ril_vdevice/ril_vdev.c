#include "ril_vdev.h"
#include "board.h"

#ifndef BSP_RIL_VDEVICE_NAME
#define BSP_RIL_VDEVICE_NAME        "ril_dev"
#endif

static struct rt_device _ril_vdev;

static rt_err_t  _ril_vdev_init(rt_device_t dev)
{
    return RT_EOK;
}

static rt_err_t  _ril_vdev_open(rt_device_t dev, rt_uint16_t oflag)
{
    if (dev == RT_NULL)
        return -RT_ERROR;

    return RT_EOK;
}

static rt_err_t _ril_vdev_close(rt_device_t dev)
{
    if (dev == RT_NULL)
        return -RT_ERROR;

    return RT_EOK;
}

rt_size_t _ril_vdev_read(rt_device_t dev, rt_off_t pos, void *buffer, rt_size_t size)
{
    rt_kprintf("%s : buffer = %s, size = %d\r\n", __func__, buffer, size);
    // XMUX_read(buffer, size);
    return RT_EOK;
}

rt_size_t _ril_vdev_write(rt_device_t dev, rt_off_t pos, const void *buffer, rt_size_t size)
{
    rt_kprintf("%s : buffer = %s, size = %d\r\n", __func__, buffer, size);
    // XMUX_write(buffer, size);
    return RT_EOK;
}

rt_err_t _ril_vdev_rx_indicate(rt_device_t dev, rt_size_t size)
{
    rt_kprintf("%s : rx_in size = %d\r\n", __func__, size);
    // XMUX_rx_indicate(size);
    return RT_EOK;
}

rt_err_t _ril_vdev_tx_complete(rt_device_t dev, void *buffer)
{
    rt_kprintf("%s : buffer=%s\r\n", __func__, buffer);
    // XMUX_tx_indicate(size);
    return RT_EOK;
}

static rt_err_t _ril_vdev_control(rt_device_t dev, int cmd, void *args)
{
    if (dev == RT_NULL)
        return -RT_ERROR;

    switch (cmd)
    {
        case RIL_VDEV_CTRL_CMD_POWER_ON:
            break;
        case RIL_VDEV_CTRL_CMD_POWER_OFF:
            break;
        default:
            break;
    }

    return RT_EOK;
}

#ifdef RT_USING_DEVICE_OPS
const static struct rt_device_ops ril_vdev_ops =
{
    _ril_vdev_init,
    _ril_vdev_open,
    _ril_vdev_close,
    _ril_vdev_read,
    _ril_vdev_write,
    _ril_vdev_control
};
#endif

static int ril_vdevice_register(const char *name, void *user_data)
{
    _ril_vdev.type         = RT_Device_Class_Char;
    _ril_vdev.rx_indicate  = _ril_vdev_rx_indicate;
    _ril_vdev.tx_complete  = _ril_vdev_tx_complete;

#ifdef RT_USING_DEVICE_OPS
    _ril_vdev.ops          = &ril_vdev_ops;
#else
    _ril_vdev.init         = _ril_vdev_init;
    _ril_vdev.open         = _ril_vdev_open;
    _ril_vdev.close        = _ril_vdev_close;
    _ril_vdev.read         = _ril_vdev_read;
    _ril_vdev.write        = _ril_vdev_write;
    _ril_vdev.control      = _ril_vdev_control;
#endif

    _ril_vdev.user_data    = user_data;

    /* register a character device */
    rt_device_register(&_ril_vdev, name, RT_DEVICE_FLAG_RDWR);

    return 0;
}

int ril_vdevice_init(void)
{
    return ril_vdevice_register(BSP_RIL_VDEVICE_NAME, RT_NULL);
}
