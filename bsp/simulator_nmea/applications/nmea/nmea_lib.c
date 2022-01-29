#include "nmea_parse.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define NMEA_TOKS_COMPARE   (1)
#define NMEA_TOKS_PERCENT   (2)
#define NMEA_TOKS_WIDTH     (3)
#define NMEA_TOKS_TYPE      (4)

/**
 * \brief Convert string to number
 */
int nmea_atoi(const char *str, int str_sz, int radix)
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
double nmea_atof(const char *str, int str_sz)
{
    char *tmp_ptr;
    char buff[NMEA_CONVSTR_BUF];
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
                memcpy(parg_target, &snum, sizeof(int));
                break;
            case 'u':
                unum = nmea_atoi(beg_tok, width, 10);
                memcpy(parg_target, &unum, sizeof(unsigned int));
                break;
            case 'x':
            case 'X':
                unum = nmea_atoi(beg_tok, width, 16);
                memcpy(parg_target, &unum, sizeof(unsigned int));
                break;
            case 'o':
                unum = nmea_atoi(beg_tok, width, 8);
                memcpy(parg_target, &unum, sizeof(unsigned int));
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

