#include "convert_time.h"

struct gps_tm {
    float tm_msec; /* m_second */
    int tm_sec; /* second �C [0,59] */
    int tm_min; /* min - [0,59] */
    int tm_hour; /* hour - [0,23] */
    int tm_mday; /* day - [1,31] */
    int tm_mon; /* month - [0,11] */
    int tm_year; /* year since 1900 */
};

/*******************************************************
*function: convert time
*return: seconds
*******************************************************/
double convert_seconds(double yymmdd, double hhmmss)
{
    struct gps_tm p_dt;
    struct tm dt;
    double result_seconds;

    p_dt.tm_year = ((int)yymmdd) % 100 + 2000 - 1900;
    p_dt.tm_mon = ((int)(yymmdd / 100)) % 100 - 1;
    p_dt.tm_mday = ((int)yymmdd) / 10000;
    p_dt.tm_hour = ((int)hhmmss) / 10000;
    p_dt.tm_min = ((int)(hhmmss / 100)) % 100;
    p_dt.tm_sec = ((int)hhmmss) % 100;
    p_dt.tm_msec = hhmmss - (long)hhmmss;

    dt.tm_year = p_dt.tm_year;
    dt.tm_mon = p_dt.tm_mon;
    dt.tm_mday = p_dt.tm_mday;
    dt.tm_hour = p_dt.tm_hour;
    dt.tm_min = p_dt.tm_min;
    dt.tm_sec = p_dt.tm_sec;

    result_seconds = (double)mktime(&dt) + p_dt.tm_msec;
    return result_seconds;
}
