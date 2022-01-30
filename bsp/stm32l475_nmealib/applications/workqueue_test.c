#include <rtthread.h>
#include <rtdevice.h>

#define DBG_ENABLE
#define DBG_SECTION_NAME    "workq.test"
#define DBG_LEVEL           DBG_LOG
#include <rtdbg.h>

#ifndef TEST_DUMP_TIME
#define TEST_DUMP_TIME          30
#endif

static struct rt_work test_work;

struct rt_timer test_timer  = { 0 };
static rt_bool_t test_timer_run_flag = RT_FALSE;
static rt_bool_t test_timer_init_flag = RT_FALSE;

/* work : do something with delay */
static void pm_log_to_file(struct rt_work *work, void *work_data)
{
    LOG_D("%s : log to file1,tick=%d", __func__, rt_tick_get());
    rt_thread_mdelay(20);
    LOG_D("%s : log to file2,tick=%d", __func__, rt_tick_get());
}

/* test timer timeout entry */
static void test_timer_timeout(void *param)
{
    rt_work_cancel(&test_work);
    rt_work_submit(&test_work, 0);
}

/* test timer init */
rt_err_t test_timer_init(void)
{
    rt_work_init(&test_work, pm_log_to_file, RT_NULL);

    if (test_timer_init_flag == RT_TRUE)
    {
        return -RT_ERROR;
    }

    rt_timer_init(&test_timer, "test_tm", test_timer_timeout, RT_NULL,
        (TEST_DUMP_TIME * RT_TICK_PER_SECOND), RT_TIMER_FLAG_PERIODIC | RT_TIMER_FLAG_SOFT_TIMER);

    test_timer_init_flag = RT_TRUE;

    return RT_TRUE;
}

/* test timer Start */
rt_err_t test_timer_start(void)
{
    rt_err_t ret = RT_EOK;

    if (test_timer_init_flag == RT_FALSE)
    {
        return -RT_ERROR;
    }

    if (test_timer_run_flag == RT_FALSE)
    {
        ret = rt_timer_start(&test_timer);
    }

    if (ret == RT_EOK)
    {
        test_timer_run_flag = RT_TRUE;
    }

    return ret;
}

/* test timer Stop*/
rt_err_t test_timer_stop(void)
{
    rt_err_t ret = RT_EOK;

    if (test_timer_init_flag == RT_FALSE)
    {
        return -RT_ERROR;
    }

    if (test_timer_run_flag == RT_TRUE)
    {
        ret = rt_timer_stop(&test_timer);
    }

    if (ret == RT_EOK)
    {
        test_timer_run_flag = RT_FALSE;
    }

    return ret;
}

void test_work_start(void)
{
    test_timer_init();
    test_timer_start();
}

void test_work_stop(void)
{
    test_timer_stop();
}

MSH_CMD_EXPORT(test_timer_init, test_timer_init);
MSH_CMD_EXPORT(test_timer_start, test_timer_start);
MSH_CMD_EXPORT(test_timer_stop, test_timer_stop);

MSH_CMD_EXPORT(test_work_start, test_work_start);
MSH_CMD_EXPORT(test_work_stop, test_work_stop);
