#include "ril_app.h"
#include "at.h"

#define DBG_ENABLE
#define DBG_SECTION_NAME    "rild.test"
#define DBG_LEVEL           DBG_LOG
#include <rtdbg.h>

#ifndef BSP_RIL_VDEVICE_NAME
#define BSP_RIL_VDEVICE_NAME        "ril_dev"
#endif

#ifndef BSP_RIL_RECV_MAX_SIZE
#define BSP_RIL_RECV_MAX_SIZE       1600
#endif

static rt_device_t _ril_dev = RT_NULL;

static rt_device_t get_ril_dev(void)
{
    if (_ril_dev != RT_NULL)
        return _ril_dev;

    _ril_dev = rt_device_find(BSP_RIL_VDEVICE_NAME);

    return _ril_dev;
}

static rt_err_t ril_dev_open(void)
{
    rt_device_t dev = get_ril_dev();
    if (dev == RT_NULL)
        return -RT_ERROR;

    return rt_device_open(dev, RT_DEVICE_FLAG_RDWR);
}

rt_err_t ril_dev_power_on(void)
{
    rt_device_t dev = get_ril_dev();

    if (dev == RT_NULL)
        return -RT_ERROR;

    return rt_device_control(dev, RIL_VDEV_CTRL_CMD_POWER_ON, RT_NULL);
}

rt_err_t ril_dev_power_off(void)
{
    rt_device_t dev = get_ril_dev();

    if (dev == RT_NULL)
        return -RT_ERROR;

    return rt_device_control(dev, RIL_VDEV_CTRL_CMD_POWER_OFF, RT_NULL);
}

rt_err_t ril_dev_init(void)
{
    ril_vdevice_init();
    return ril_dev_open();
}

int ril_at_device_init(void)
{
    ril_dev_init();
    at_client_init(BSP_RIL_VDEVICE_NAME, BSP_RIL_RECV_MAX_SIZE);

    return 0;
}

static void at_response_dump(at_response_t resp)
{
    rt_uint8_t line_num = 0;

    LOG_D("%s: line_counts=%d\n", __func__, resp->line_counts);

    for (line_num = 1; line_num <= resp->line_counts; line_num++)
    {
        const char *resp_line_buf = at_resp_get_line(resp, line_num);

        if (resp_line_buf != RT_NULL)
        {
            LOG_D("%s:line=%d, buf=%s\n", __func__, line_num, resp_line_buf);
        }
    }
}

rt_err_t ril_test_check_sim(void)
{
    at_response_t resp = RT_NULL;

    resp = at_create_resp(64, 3, rt_tick_from_millisecond(500));
    if (resp == RT_NULL)
    {
        LOG_E("%s:at_create_resp error!", __func__);
        return -RT_ERROR;
    }

    at_exec_cmd(resp, "AT+CPIN?");
    at_response_dump(resp);

    if (at_resp_get_line_by_kw(resp, "READY"))
    {
        LOG_D("at_check_sim success.");
        if (resp != RT_NULL)
            at_delete_resp(resp);
        return RT_EOK;
    }

    if (resp != RT_NULL)
        at_delete_resp(resp);
    LOG_E("at_check_sim failed.");
    return -RT_ERROR;
}
MSH_CMD_EXPORT(ril_test_check_sim, ril_test_check_sim);

/* ril_test_at_cmd */
static void ril_test_at_cmd(int argc, char **argv)
{
    at_response_t resp = RT_NULL;

    char *at_cmd = RT_NULL;

    if (argc >= 2)
    {
        at_cmd = argv[1];
    }

    resp = at_create_resp(1024, 4, rt_tick_from_millisecond(1000));
    if (resp == RT_NULL)
    {
        LOG_E("%s:at_create_resp error!", __func__);
        return;
    }

    at_exec_cmd(resp, at_cmd);
    at_response_dump(resp);

    if (resp != RT_NULL)
        at_delete_resp(resp);
}
MSH_CMD_EXPORT(ril_test_at_cmd, ril_test_at_cmd);

INIT_DEVICE_EXPORT(ril_at_device_init);
