#include <nmea_parse.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define NMEA_TOKS_COMPARE   (1)
#define NMEA_TOKS_PERCENT   (2)
#define NMEA_TOKS_WIDTH     (3)
#define NMEA_TOKS_TYPE      (4)

typedef struct _nmea_parser_node
{
    int packType;
    void *pack;
    struct _nmea_parser_node *next_node;
} nmea_parser_node_t;

static int nmea_parser_queue_clear(nmea_parser_t *parser);
static int nmea_parser_push(nmea_parser_t *parser, const char *buff, int buff_sz);
static int nmea_parser_pop(nmea_parser_t *parser, void **pack_ptr);
static int nmea_parser_buff_clear(nmea_parser_t *parser);

 /**
  * \brief Initialization of parser object
  * @return true (1) - success or false (0) - fail
  */
int nmea_parser_init(nmea_parser_t *parser)
{
    int resv = 0;
    int buff_size = NMEA_DEF_PARSEBUFF;

    NMEA_ASSERT(parser);

    if (buff_size < NMEA_MIN_PARSEBUFF)
        buff_size = NMEA_MIN_PARSEBUFF;

    rt_memset(parser, 0, sizeof(nmea_parser_t));
    parser->buffer = rt_malloc(buff_size);
    if (RT_NULL == parser->buffer)
    {
        nmea_error("Insufficient memory!");
        resv = -1;
    }
    else
    {
        parser->buff_size = buff_size;
        resv = 1;
    }

    return resv;
}

/**
 * \brief Destroy parser object
 */
void nmea_parser_destroy(nmea_parser_t *parser)
{
    NMEA_ASSERT(parser && parser->buffer);
    rt_free(parser->buffer);
    nmea_parser_queue_clear(parser);
    rt_memset(parser, 0, sizeof(nmea_parser_t));
}

int _nmea_parse_time(const char *buff, int buff_sz, nmea_time_t *res)
{
    int success = 0;

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
        nmea_error("Parse of time error (format error)!");
        success = 0;
        break;
    }

    return (success ? 0 : -1);
}

/**
 * \brief Define packet type by header (NMEA_PACK_TYPE).
 * @param buff a constant character pointer of packet buffer.
 * @param buff_sz buffer size.
 * @return The defined packet type
 * @see NMEA_PACK_TYPE
 */
int nmea_pack_type(const char *buff, int buff_sz)
{
    static const char *pheads[] = {
        "GPGGA",
        "GPGSA",
        "GPGSV",
        "GPRMC",
        "GPVTG",
    };

    NMEA_ASSERT(buff);

    if (buff_sz < 5)
        return GPNON;
    else if (0 == rt_memcmp(buff, pheads[0], 5))
        return GPGGA;
    else if (0 == rt_memcmp(buff, pheads[1], 5))
        return GPGSA;
    else if (0 == rt_memcmp(buff, pheads[2], 5))
        return GPGSV;
    else if (0 == rt_memcmp(buff, pheads[3], 5))
        return GPRMC;
    else if (0 == rt_memcmp(buff, pheads[4], 5))
        return GPVTG;

    return GPNON;
}

/**
 * \brief Find tail of packet ("\r\n") in buffer and check control sum (CRC).
 * @param buff a constant character pointer of packets buffer.
 * @param buff_sz buffer size.
 * @param res_crc a integer pointer for return CRC of packet (must be defined).
 * @return Number of bytes to packet tail.
 */
int nmea_find_tail(const char *buff, int buff_sz, int *res_crc)
{
    static const int tail_sz = 3 /* *[CRC] */ + 2 /* \r\n */;

    const char *end_buff = buff + buff_sz;
    int nread = 0;
    int crc = 0;

    NMEA_ASSERT(buff && res_crc);

    *res_crc = -1;

    for (; buff < end_buff; ++buff, ++nread)
    {
        if (('$' == *buff) && nread)
        {
            buff = 0;
            break;
        }
        else if ('*' == *buff)
        {
            if (buff + tail_sz <= end_buff && '\r' == buff[3] && '\n' == buff[4])
            {
                *res_crc = nmea_atoi(buff + 1, 2, 16);
                nread = buff_sz - (int)(end_buff - (buff + tail_sz));
                if (*res_crc != crc)
                {
                    *res_crc = -1;
                    buff = 0;
                }
            }

            break;
        }
        else if (nread)
            crc ^= (int)*buff;
    }

    if (*res_crc < 0 && buff)
        nread = 0;

    return nread;
}

/**
 * \brief Parse GGA packet from buffer.
 * @param buff a constant character pointer of packet buffer.
 * @param buff_sz buffer size.
 * @param pack a pointer of packet which will filled by function.
 * @return 1 (true) - if parsed successfully or 0 (false) - if fail.
 */
int nmea_parse_gga(const char *buff, int buff_sz, nmea_gga_t *pack)
{
    char time_buff[NMEA_TIMEPARSE_BUF];

    NMEA_ASSERT(buff && pack);

    rt_memset(pack, 0, sizeof(nmea_gga_t));

    if (14 != nmea_scanf(buff, buff_sz,
        "$GPGGA,%s,%f,%C,%f,%C,%d,%d,%f,%f,%C,%f,%C,%f,%d*",
        &(time_buff[0]),
        &(pack->lat), &(pack->ns), &(pack->lon), &(pack->ew),
        &(pack->sig), &(pack->satinuse), &(pack->HDOP), &(pack->elv), &(pack->elv_units),
        &(pack->diff), &(pack->diff_units), &(pack->dgps_age), &(pack->dgps_sid)))
    {
        nmea_error("GPGGA parse error!");
        return 0;
    }

    if (0 != _nmea_parse_time(&time_buff[0], (int)rt_strlen(&time_buff[0]), &(pack->utc)))
    {
        nmea_error("GPGGA time parse error!");
        return 0;
    }

    return 1;
}

/**
 * \brief Parse GSA packet from buffer.
 * @param buff a constant character pointer of packet buffer.
 * @param buff_sz buffer size.
 * @param pack a pointer of packet which will filled by function.
 * @return 1 (true) - if parsed successfully or 0 (false) - if fail.
 */
int nmea_parse_gsa(const char *buff, int buff_sz, nmea_gsa_t *pack)
{
    NMEA_ASSERT(buff && pack);

    rt_memset(pack, 0, sizeof(nmea_gsa_t));

    if (17 != nmea_scanf(buff, buff_sz,
        "$GPGSA,%C,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%f,%f,%f*",
        &(pack->fix_mode), &(pack->fix_type),
        &(pack->sat_prn[0]), &(pack->sat_prn[1]), &(pack->sat_prn[2]), &(pack->sat_prn[3]), &(pack->sat_prn[4]), &(pack->sat_prn[5]),
        &(pack->sat_prn[6]), &(pack->sat_prn[7]), &(pack->sat_prn[8]), &(pack->sat_prn[9]), &(pack->sat_prn[10]), &(pack->sat_prn[11]),
        &(pack->PDOP), &(pack->HDOP), &(pack->VDOP)))
    {
        nmea_error("GPGSA parse error!");
        return 0;
    }

    return 1;
}

/**
 * \brief Parse GSV packet from buffer.
 * @param buff a constant character pointer of packet buffer.
 * @param buff_sz buffer size.
 * @param pack a pointer of packet which will filled by function.
 * @return 1 (true) - if parsed successfully or 0 (false) - if fail.
 */
int nmea_parse_gsv(const char *buff, int buff_sz, nmea_gsv_t *pack)
{
    int nsen, nsat;

    NMEA_ASSERT(buff && pack);

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
        nmea_error("GPGSV parse error!");
        return 0;
    }

    return 1;
}

/**
 * \brief Parse RMC packet from buffer.
 * @param buff a constant character pointer of packet buffer.
 * @param buff_sz buffer size.
 * @param pack a pointer of packet which will filled by function.
 * @return 1 (true) - if parsed successfully or 0 (false) - if fail.
 */
int nmea_parse_rmc(const char *buff, int buff_sz, nmea_rmc_t *pack)
{
    int nsen;
    char time_buff[NMEA_TIMEPARSE_BUF];

    NMEA_ASSERT(buff && pack);

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
        nmea_error("GPRMC parse error!");
        return 0;
    }

    if (0 != _nmea_parse_time(&time_buff[0], (int)rt_strlen(&time_buff[0]), &(pack->utc)))
    {
        nmea_error("GPRMC time parse error!");
        return 0;
    }

    if (pack->utc.year < 90)
        pack->utc.year += 100;
    pack->utc.mon -= 1;

    return 1;
}

/**
 * \brief Parse VTG packet from buffer.
 * @param buff a constant character pointer of packet buffer.
 * @param buff_sz buffer size.
 * @param pack a pointer of packet which will filled by function.
 * @return 1 (true) - if parsed successfully or 0 (false) - if fail.
 */
int nmea_parse_vtg(const char *buff, int buff_sz, nmea_vtg_t *pack)
{
    NMEA_ASSERT(buff && pack);

    rt_memset(pack, 0, sizeof(nmea_vtg_t));

    if (8 != nmea_scanf(buff, buff_sz,
        "$GPVTG,%f,%C,%f,%C,%f,%C,%f,%C*",
        &(pack->dir), &(pack->dir_t),
        &(pack->dec), &(pack->dec_m),
        &(pack->spn), &(pack->spn_n),
        &(pack->spk), &(pack->spk_k)))
    {
        nmea_error("GPVTG parse error!");
        return 0;
    }

    if (pack->dir_t != 'T' ||
        pack->dec_m != 'M' ||
        pack->spn_n != 'N' ||
        pack->spk_k != 'K')
    {
        nmea_error("GPVTG parse error (format error)!");
        return 0;
    }

    return 1;
}

/**
 * \brief Fill nmea_info_t structure by GGA packet data.
 * @param pack a pointer of packet structure.
 * @param info a pointer of summary information structure.
 */
void nmea_gga_info(nmea_gga_t *pack, nmea_info_t *info)
{
    NMEA_ASSERT(pack && info);

    info->utc.hour = pack->utc.hour;
    info->utc.min = pack->utc.min;
    info->utc.sec = pack->utc.sec;
    info->utc.hsec = pack->utc.hsec;
    info->sig = pack->sig;
    info->HDOP = pack->HDOP;
    info->elv = pack->elv;
    info->lat = ((pack->ns == 'N') ? pack->lat : -(pack->lat));
    info->lon = ((pack->ew == 'E') ? pack->lon : -(pack->lon));
    info->smask |= GPGGA;
}

/**
 * \brief Fill nmea_info_t structure by GSA packet data.
 * @param pack a pointer of packet structure.
 * @param info a pointer of summary information structure.
 */
void nmea_gsa_info(nmea_gsa_t *pack, nmea_info_t *info)
{
    int i, j, nuse = 0;

    NMEA_ASSERT(pack && info);

    info->fix = pack->fix_type;
    info->PDOP = pack->PDOP;
    info->HDOP = pack->HDOP;
    info->VDOP = pack->VDOP;

    for (i = 0; i < NMEA_MAXSAT; ++i)
    {
        for (j = 0; j < info->satinfo.inview; ++j)
        {
            if (pack->sat_prn[i] && pack->sat_prn[i] == info->satinfo.sat[j].id)
            {
                info->satinfo.sat[j].in_use = 1;
                nuse++;
            }
        }
    }

    info->satinfo.inuse = nuse;
    info->smask |= GPGSA;
}

/**
 * \brief Fill nmea_info_t structure by GSV packet data.
 * @param pack a pointer of packet structure.
 * @param info a pointer of summary information structure.
 */
void nmea_gsv_info(nmea_gsv_t *pack, nmea_info_t *info)
{
    int isat, isi, nsat;

    NMEA_ASSERT(pack && info);

    if (pack->pack_index > pack->pack_count ||
        pack->pack_index * NMEA_SATINPACK > NMEA_MAXSAT)
        return;

    if (pack->pack_index < 1)
        pack->pack_index = 1;

    info->satinfo.inview = pack->sat_count;

    nsat = (pack->pack_index - 1) * NMEA_SATINPACK;
    nsat = (nsat + NMEA_SATINPACK > pack->sat_count) ? pack->sat_count - nsat : NMEA_SATINPACK;

    for (isat = 0; isat < nsat; ++isat)
    {
        isi = (pack->pack_index - 1) * NMEA_SATINPACK + isat;
        info->satinfo.sat[isi].id = pack->sat_data[isat].id;
        info->satinfo.sat[isi].elv = pack->sat_data[isat].elv;
        info->satinfo.sat[isi].azimuth = pack->sat_data[isat].azimuth;
        info->satinfo.sat[isi].sig = pack->sat_data[isat].sig;
    }

    info->smask |= GPGSV;
}

/**
 * \brief Fill nmea_info_t structure by RMC packet data.
 * @param pack a pointer of packet structure.
 * @param info a pointer of summary information structure.
 */
void nmea_rmc_info(nmea_rmc_t *pack, nmea_info_t *info)
{
    NMEA_ASSERT(pack && info);

    if ('A' == pack->status)
    {
        if (NMEA_SIG_BAD == info->sig)
            info->sig = NMEA_SIG_MID;
        if (NMEA_FIX_BAD == info->fix)
            info->fix = NMEA_FIX_2D;
    }
    else if ('V' == pack->status)
    {
        info->sig = NMEA_SIG_BAD;
        info->fix = NMEA_FIX_BAD;
    }

    info->utc = pack->utc;
    info->lat = ((pack->ns == 'N') ? pack->lat : -(pack->lat));
    info->lon = ((pack->ew == 'E') ? pack->lon : -(pack->lon));
    info->speed = pack->speed * NMEA_TUD_KNOTS;
    info->direction = pack->direction;
    info->smask |= GPRMC;
}

/**
 * \brief Fill nmea_info_t structure by VTG packet data.
 * @param pack a pointer of packet structure.
 * @param info a pointer of summary information structure.
 */
void nmea_vtg_info(nmea_vtg_t *pack, nmea_info_t *info)
{
    NMEA_ASSERT(pack && info);

    info->direction = pack->dir;
    info->declination = pack->dec;
    info->speed = pack->spk;
    info->smask |= GPVTG;
}

/**
 * \brief Analysis of buffer and put results to information structure
 * @return Number of packets wos parsed
 */
int nmea_parse(
    nmea_parser_t *parser,
    const char *buff, int buff_sz,
    nmea_info_t *info
)
{
    int ptype, nread = 0;
    void *pack = 0;

    NMEA_ASSERT(parser && parser->buffer);

    nmea_parser_push(parser, buff, buff_sz);

    while (GPNON != (ptype = nmea_parser_pop(parser, &pack)))
    {
        nread++;

        switch (ptype)
        {
        case GPGGA:
            nmea_gga_info((nmea_gga_t *)pack, info);
            break;
        case GPGSA:
            nmea_gsa_info((nmea_gsa_t *)pack, info);
            break;
        case GPGSV:
            nmea_gsv_info((nmea_gsv_t *)pack, info);
            break;
        case GPRMC:
            nmea_rmc_info((nmea_rmc_t *)pack, info);
            break;
        case GPVTG:
            nmea_vtg_info((nmea_vtg_t *)pack, info);
            break;
        };

        rt_free(pack);
    }

    return nread;
}

/*
 * low level
 */

int nmea_parser_real_push(nmea_parser_t *parser, const char *buff, int buff_sz)
{
    int nparsed = 0, crc, sen_sz, ptype;
    nmea_parser_node_t *node = 0;

    NMEA_ASSERT(parser && parser->buffer);

    /* clear unuse buffer (for debug) */
    /*
    memset(
        parser->buffer + parser->buff_use, 0,
        parser->buff_size - parser->buff_use
        );
        */

        /* add */
    if (parser->buff_use + buff_sz >= parser->buff_size)
        nmea_parser_buff_clear(parser);

    rt_memcpy(parser->buffer + parser->buff_use, buff, buff_sz);
    parser->buff_use += buff_sz;

    /* parse */
    for (;; node = 0)
    {
        sen_sz = nmea_find_tail(
            (const char *)parser->buffer + nparsed,
            (int)parser->buff_use - nparsed, &crc);

        if (!sen_sz)
        {
            if (nparsed)
                rt_memcpy(
                    parser->buffer,
                    parser->buffer + nparsed,
                    parser->buff_use -= nparsed);
            break;
        }
        else if (crc >= 0)
        {
            ptype = nmea_pack_type(
                (const char *)parser->buffer + nparsed + 1,
                parser->buff_use - nparsed - 1);

            if (0 == (node = rt_malloc(sizeof(nmea_parser_node_t))))
                goto mem_fail;

            node->pack = 0;

            switch (ptype)
            {
            case GPGGA:
                if (0 == (node->pack = rt_malloc(sizeof(nmea_gga_t))))
                    goto mem_fail;
                node->packType = GPGGA;
                if (!nmea_parse_gga(
                    (const char *)parser->buffer + nparsed,
                    sen_sz, (nmea_gga_t *)node->pack))
                {
                    rt_free(node);
                    node = 0;
                }
                break;
            case GPGSA:
                if (0 == (node->pack = rt_malloc(sizeof(nmea_gsa_t))))
                    goto mem_fail;
                node->packType = GPGSA;
                if (!nmea_parse_gsa(
                    (const char *)parser->buffer + nparsed,
                    sen_sz, (nmea_gsa_t *)node->pack))
                {
                    rt_free(node);
                    node = 0;
                }
                break;
            case GPGSV:
                if (0 == (node->pack = rt_malloc(sizeof(nmea_gsv_t))))
                    goto mem_fail;
                node->packType = GPGSV;
                if (!nmea_parse_gsv(
                    (const char *)parser->buffer + nparsed,
                    sen_sz, (nmea_gsv_t *)node->pack))
                {
                    rt_free(node);
                    node = 0;
                }
                break;
            case GPRMC:
                if (0 == (node->pack = rt_malloc(sizeof(nmea_rmc_t))))
                    goto mem_fail;
                node->packType = GPRMC;
                if (!nmea_parse_rmc(
                    (const char *)parser->buffer + nparsed,
                    sen_sz, (nmea_rmc_t *)node->pack))
                {
                    rt_free(node);
                    node = 0;
                }
                break;
            case GPVTG:
                if (0 == (node->pack = rt_malloc(sizeof(nmea_vtg_t))))
                    goto mem_fail;
                node->packType = GPVTG;
                if (!nmea_parse_vtg(
                    (const char *)parser->buffer + nparsed,
                    sen_sz, (nmea_vtg_t *)node->pack))
                {
                    rt_free(node);
                    node = 0;
                }
                break;
            default:
                rt_free(node);
                node = 0;
                break;
            };

            if (node)
            {
                if (parser->end_node)
                    ((nmea_parser_node_t *)parser->end_node)->next_node = node;
                parser->end_node = node;
                if (!parser->top_node)
                    parser->top_node = node;
                node->next_node = 0;
            }
        }

        nparsed += sen_sz;
    }

    return nparsed;

mem_fail:
    if (node)
        rt_free(node);

    nmea_error("Insufficient memory!");

    return -1;
}

/**
 * \brief Analysis of buffer and keep results into parser
 * @return Number of bytes wos parsed from buffer
 */
static int nmea_parser_push(nmea_parser_t *parser, const char *buff, int buff_sz)
{
    int nparse, nparsed = 0;

    do
    {
        if (buff_sz > parser->buff_size)
            nparse = parser->buff_size;
        else
            nparse = buff_sz;

        nparsed += nmea_parser_real_push(parser, buff, nparse);

        buff_sz -= nparse;

    } while (buff_sz);

    return nparsed;
}

/**
 * \brief Get type of top packet keeped into parser
 * @return Type of packet
 * @see NMEA_PACK_TYPE
 */
int nmea_parser_top(nmea_parser_t *parser)
{
    int retval = GPNON;
    nmea_parser_node_t *node = (nmea_parser_node_t *)parser->top_node;

    NMEA_ASSERT(parser && parser->buffer);

    if (node)
        retval = node->packType;

    return retval;
}

/**
 * \brief Withdraw top packet from parser
 * @return Received packet type
 * @see NMEA_PACK_TYPE
 */
static int nmea_parser_pop(nmea_parser_t *parser, void **pack_ptr)
{
    int retval = GPNON;
    nmea_parser_node_t *node = (nmea_parser_node_t *)parser->top_node;

    NMEA_ASSERT(parser && parser->buffer);

    if (node)
    {
        *pack_ptr = node->pack;
        retval = node->packType;
        parser->top_node = node->next_node;
        if (!parser->top_node)
            parser->end_node = 0;
        rt_free(node);
    }

    return retval;
}

/**
 * \brief Get top packet from parser without withdraw
 * @return Received packet type
 * @see NMEA_PACK_TYPE
 */
int nmea_parser_peek(nmea_parser_t *parser, void **pack_ptr)
{
    int retval = GPNON;
    nmea_parser_node_t *node = (nmea_parser_node_t *)parser->top_node;

    NMEA_ASSERT(parser && parser->buffer);

    if (node)
    {
        *pack_ptr = node->pack;
        retval = node->packType;
    }

    return retval;
}

/**
 * \brief Delete top packet from parser
 * @return Deleted packet type
 * @see NMEA_PACK_TYPE
 */
int nmea_parser_drop(nmea_parser_t *parser)
{
    int retval = GPNON;
    nmea_parser_node_t *node = (nmea_parser_node_t *)parser->top_node;

    NMEA_ASSERT(parser && parser->buffer);

    if (node)
    {
        if (node->pack)
            rt_free(node->pack);
        retval = node->packType;
        parser->top_node = node->next_node;
        if (!parser->top_node)
            parser->end_node = 0;
        rt_free(node);
    }

    return retval;
}

/**
 * \brief Clear cache of parser
 * @return true (1) - success
 */
static int nmea_parser_buff_clear(nmea_parser_t *parser)
{
    NMEA_ASSERT(parser && parser->buffer);
    parser->buff_use = 0;
    return 1;
}

/**
 * \brief Clear packets queue into parser
 * @return true (1) - success
 */
static int nmea_parser_queue_clear(nmea_parser_t *parser)
{
    NMEA_ASSERT(parser);
    while (parser->top_node)
        nmea_parser_drop(parser);
    return 1;
}

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
