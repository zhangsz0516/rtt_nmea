#include <nmea_parse.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define DBG_TAG    "nmea.parse"
#ifdef PW_NMEA_USING_DEBUG
#define DBG_LVL     DBG_LOG
#else
#define DBG_LVL     DBG_INFO
#endif
#include <rtdbg.h>

#define NMEA_TOKS_COMPARE   (1)
#define NMEA_TOKS_PERCENT   (2)
#define NMEA_TOKS_WIDTH     (3)
#define NMEA_TOKS_TYPE      (4)

/**
 * \brief Convert string to number
 */
static int nmea_atoi(const char *str, int str_sz, int radix)
{
    char *tmp_ptr;
    char buff[NMEA_CONVSTR_BUF];
    int res = 0;

    if (str_sz < NMEA_CONVSTR_BUF)
    {
        rt_memcpy(&buff[0], str, str_sz);
        buff[str_sz] = '\0';
        res = strtol(&buff[0], &tmp_ptr, radix);
    }

    return res;
}

/**
 * \brief Convert string to fraction number
 */
static double nmea_atof(const char *str, int str_sz)
{
    char *tmp_ptr = RT_NULL;
    char buff[NMEA_CONVSTR_BUF] = { 0 };
    double res = 0;

    if (str_sz < NMEA_CONVSTR_BUF)
    {
        rt_memcpy(&buff[0], str, str_sz);
        buff[str_sz] = '\0';
        res = strtod(&buff[0], &tmp_ptr);
    }

    return res;
}

/**
 * \brief Analyse string (specificate for NMEA sentences)
 */
int nmea_scanf(const char *buff, int buff_sz, const char *format, ...)
{
    const char *beg_tok;
    const char *end_buf = buff + buff_sz;

    va_list arg_ptr;
    int tok_type = NMEA_TOKS_COMPARE;
    int width = 0;
    const char *beg_fmt = 0;
    int snum = 0, unum = 0;

    int tok_count = 0;
    void *parg_target;

    va_start(arg_ptr, format);

    for (; *format && buff < end_buf; ++format)
    {
        switch (tok_type)
        {
        case NMEA_TOKS_COMPARE:
            if ('%' == *format)
                tok_type = NMEA_TOKS_PERCENT;
            else if (*buff++ != *format)
                goto fail;
            break;
        case NMEA_TOKS_PERCENT:
            width = 0;
            beg_fmt = format;
            tok_type = NMEA_TOKS_WIDTH;
        case NMEA_TOKS_WIDTH:
            if (isdigit(*format))
                break;
            {
                tok_type = NMEA_TOKS_TYPE;
                if (format > beg_fmt)
                    width = nmea_atoi(beg_fmt, (int)(format - beg_fmt), 10);
            }
        case NMEA_TOKS_TYPE:
            beg_tok = buff;

            if (!width && ('c' == *format || 'C' == *format) && *buff != format[1])
                width = 1;

            if (width)
            {
                if (buff + width <= end_buf)
                    buff += width;
                else
                    goto fail;
            }
            else
            {
                if (!format[1] || (0 == (buff = (char *)memchr(buff, format[1], end_buf - buff))))
                    buff = end_buf;
            }

            if (buff > end_buf)
                goto fail;

            tok_type = NMEA_TOKS_COMPARE;
            tok_count++;

            parg_target = 0; width = (int)(buff - beg_tok);

            switch (*format)
            {
            case 'c':
            case 'C':
                parg_target = (void *)va_arg(arg_ptr, char *);
                if (width && 0 != (parg_target))
                    *((char *)parg_target) = *beg_tok;
                break;
            case 's':
            case 'S':
                parg_target = (void *)va_arg(arg_ptr, char *);
                if (width && 0 != (parg_target))
                {
                    rt_memcpy(parg_target, beg_tok, width);
                    ((char *)parg_target)[width] = '\0';
                }
                break;
            case 'f':
            case 'g':
            case 'G':
            case 'e':
            case 'E':
                parg_target = (void *)va_arg(arg_ptr, double *);
                if (width && 0 != (parg_target))
                    *((double *)parg_target) = nmea_atof(beg_tok, width);
                break;
            };

            if (parg_target)
                break;
            if (0 == (parg_target = (void *)va_arg(arg_ptr, int *)))
                break;
            if (!width)
                break;

            switch (*format)
            {
            case 'd':
            case 'i':
                snum = nmea_atoi(beg_tok, width, 10);
                rt_memcpy(parg_target, &snum, sizeof(int));
                break;
            case 'u':
                unum = nmea_atoi(beg_tok, width, 10);
                rt_memcpy(parg_target, &unum, sizeof(unsigned int));
                break;
            case 'x':
            case 'X':
                unum = nmea_atoi(beg_tok, width, 16);
                rt_memcpy(parg_target, &unum, sizeof(unsigned int));
                break;
            case 'o':
                unum = nmea_atoi(beg_tok, width, 8);
                rt_memcpy(parg_target, &unum, sizeof(unsigned int));
                break;
            default:
                goto fail;
            };

            break;
        };
    }

fail:

    va_end(arg_ptr);

    return tok_count;
}

static rt_err_t _nmea_parse_time(const char *buff, int buff_sz, nmea_time_t *res)
{
    int success = 0;

    if (buff == RT_NULL)
    {
        LOG_E("%s : buff is NULL!", __func__);
        return -RT_ERROR;
    }

    switch (buff_sz)
    {
    case sizeof("hhmmss") - 1:
        success = (3 == nmea_scanf(buff, buff_sz,
            "%2d%2d%2d", &(res->hour), &(res->min), &(res->sec)
        ));
        break;
    case sizeof("hhmmss.s") - 1:
    case sizeof("hhmmss.ss") - 1:
    case sizeof("hhmmss.sss") - 1:
        success = (4 == nmea_scanf(buff, buff_sz,
            "%2d%2d%2d.%d", &(res->hour), &(res->min), &(res->sec), &(res->hsec)
        ));
        break;
    default:
        LOG_E("Parse of time error (format error)!");
        success = 0;
        break;
    }

    return (success ? RT_EOK : -RT_ERROR);
}

rt_err_t nmea_parse_gga(const char *buff, int buff_sz, nmea_gga_t *pack)
{
    char time_buff[NMEA_TIMEPARSE_BUF] = { 0 };

    if ((buff == RT_NULL) || (pack == RT_NULL))
    {
        LOG_E("%s : buff or pack is NULL!", __func__);
        return -RT_ERROR;
    }

    rt_memset(pack, 0, sizeof(nmea_gga_t));

    if (14 != nmea_scanf(buff, buff_sz,
        "$GPGGA,%s,%f,%C,%f,%C,%d,%d,%f,%f,%C,%f,%C,%f,%d*",
        &(time_buff[0]),
        &(pack->lat), &(pack->ns), &(pack->lon), &(pack->ew),
        &(pack->sig), &(pack->satinuse), &(pack->HDOP), &(pack->elv), &(pack->elv_units),
        &(pack->diff), &(pack->diff_units), &(pack->dgps_age), &(pack->dgps_sid)))
    {
        LOG_E("%s : parse error!", __func__);
        return -RT_ERROR;
    }

    if (0 != _nmea_parse_time(&time_buff[0], (int)rt_strlen(&time_buff[0]), &(pack->utc)))
    {
        LOG_E("%s : nmea_time parse error!", __func__);
        return -RT_ERROR;
    }

    return RT_EOK;
}

rt_err_t nmea_parse_gsa(const char *buff, int buff_sz, nmea_gsa_t *pack)
{
    if ((buff == RT_NULL) || (pack == RT_NULL))
    {
        LOG_E("%s : buff or pack is NULL!", __func__);
        return -RT_ERROR;
    }

    rt_memset(pack, 0, sizeof(nmea_gsa_t));

    if (17 != nmea_scanf(buff, buff_sz,
        "$GPGSA,%C,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%f,%f,%f*",
        &(pack->fix_mode), &(pack->fix_type),
        &(pack->sat_prn[0]), &(pack->sat_prn[1]), &(pack->sat_prn[2]), &(pack->sat_prn[3]), &(pack->sat_prn[4]), &(pack->sat_prn[5]),
        &(pack->sat_prn[6]), &(pack->sat_prn[7]), &(pack->sat_prn[8]), &(pack->sat_prn[9]), &(pack->sat_prn[10]), &(pack->sat_prn[11]),
        &(pack->PDOP), &(pack->HDOP), &(pack->VDOP)))
    {
        LOG_E("%s : parse error!", __func__);
        return -RT_ERROR;
    }

    return RT_EOK;
}

rt_err_t nmea_parse_gsv(const char *buff, int buff_sz, nmea_gsv_t *pack)
{
    int nsen = 0;
    int nsat = 0;

    if ((buff == RT_NULL) || (pack == RT_NULL))
    {
        LOG_E("%s : buff or pack is NULL!", __func__);
        return -RT_ERROR;
    }

    rt_memset(pack, 0, sizeof(nmea_gsv_t));

    nsen = nmea_scanf(buff, buff_sz,
        "$GPGSV,%d,%d,%d,"
        "%d,%d,%d,%d,"
        "%d,%d,%d,%d,"
        "%d,%d,%d,%d,"
        "%d,%d,%d,%d*",
        &(pack->pack_count), &(pack->pack_index), &(pack->sat_count),
        &(pack->sat_data[0].id), &(pack->sat_data[0].elv), &(pack->sat_data[0].azimuth), &(pack->sat_data[0].sig),
        &(pack->sat_data[1].id), &(pack->sat_data[1].elv), &(pack->sat_data[1].azimuth), &(pack->sat_data[1].sig),
        &(pack->sat_data[2].id), &(pack->sat_data[2].elv), &(pack->sat_data[2].azimuth), &(pack->sat_data[2].sig),
        &(pack->sat_data[3].id), &(pack->sat_data[3].elv), &(pack->sat_data[3].azimuth), &(pack->sat_data[3].sig));

    nsat = (pack->pack_index - 1) * NMEA_SATINPACK;
    nsat = (nsat + NMEA_SATINPACK > pack->sat_count) ? pack->sat_count - nsat : NMEA_SATINPACK;
    nsat = nsat * 4 + 3 /* first three sentence`s */;

    if (nsen < nsat || nsen >(NMEA_SATINPACK * 4 + 3))
    {
        LOG_E("%s : parse error!", __func__);
        return -RT_ERROR;
    }

    return RT_EOK;
}

rt_err_t nmea_parse_rmc(const char *buff, int buff_sz, nmea_rmc_t *pack)
{
    int nsen = -1;
    char time_buff[NMEA_TIMEPARSE_BUF] = { 0 };

    if ((buff == RT_NULL) || (pack == RT_NULL))
    {
        LOG_E("%s : buff or pack is NULL!", __func__);
        return -RT_ERROR;
    }

    rt_memset(pack, 0, sizeof(nmea_rmc_t));

    nsen = nmea_scanf(buff, buff_sz,
        "$GPRMC,%s,%C,%f,%C,%f,%C,%f,%f,%2d%2d%2d,%f,%C,%C*",
        &(time_buff[0]),
        &(pack->status), &(pack->lat), &(pack->ns), &(pack->lon), &(pack->ew),
        &(pack->speed), &(pack->direction),
        &(pack->utc.day), &(pack->utc.mon), &(pack->utc.year),
        &(pack->declination), &(pack->declin_ew), &(pack->mode));

    if (nsen != 13 && nsen != 14)
    {
        LOG_E("%s : parse error!", __func__);
        return -RT_ERROR;
    }

    if (0 != _nmea_parse_time(&time_buff[0], (int)rt_strlen(&time_buff[0]), &(pack->utc)))
    {
        LOG_E("%s : time parse error!", __func__);
        return -RT_ERROR;
    }

    if (pack->utc.year < 90)
        pack->utc.year += 100;
    pack->utc.mon -= 1;

    return RT_EOK;
}

rt_err_t nmea_parse_vtg(const char *buff, int buff_sz, nmea_vtg_t *pack)
{
    if ((buff == RT_NULL) || (pack == RT_NULL))
    {
        LOG_E("%s : buff or pack is NULL!", __func__);
        return -RT_ERROR;
    }

    rt_memset(pack, 0, sizeof(nmea_vtg_t));

    if (8 != nmea_scanf(buff, buff_sz,
        "$GPVTG,%f,%C,%f,%C,%f,%C,%f,%C*",
        &(pack->dir), &(pack->dir_t),
        &(pack->dec), &(pack->dec_m),
        &(pack->spn), &(pack->spn_n),
        &(pack->spk), &(pack->spk_k)))
    {
        LOG_E("%s : parse error!", __func__);
        return -RT_ERROR;
    }

    if (pack->dir_t != 'T' ||
        pack->dec_m != 'M' ||
        pack->spn_n != 'N' ||
        pack->spk_k != 'K')
    {
        LOG_E("%s : parse format error!", __func__);
        return -RT_ERROR;
    }

    return RT_EOK;
}
