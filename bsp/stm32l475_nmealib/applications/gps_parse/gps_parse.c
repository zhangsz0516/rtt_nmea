#include "gps_parse.h"
//#include "gps_operation.h"
//#include "ephemeris_timestamp.h"
//#include "gnss_epo.h"
#include <rtthread.h>

#include <stdlib.h>

#undef LOG_TAG
#define LOG_TAG "GPS"
#include <rtdbg.h>

static struct NMEA_PARSE g_nmea_parse = { 0 };

/***********************************************************************************
* gps_parse.c module description is internal gps operation API.
* this uint functions is used to parse gps position data througt NMEA_1083 Standard
* notes: this module prohibit changed througt other module.
*
************************************************************************************/


//uint8_t data_conversion(char *input, char **output, uint8_t section_max_count);
static double latitude_longitude_conversion(double fData);


/*******************************************************
*function description:  NMEA data separator
*
*return:
*******************************************************/
static double latitude_longitude_conversion(double fData)
{
    double result_val = 0.000000;
    int32_t i = 0;
    i = fData / 100;
    result_val = i + ((fData / 100 - i) * 100) / 60;
    return result_val;
}

/*******************************************************
*function description:  NMEA data separator
*
*return:
*******************************************************/
uint8_t data_conversion(char *input, char **output, uint8_t section_max_count)
{
    uint8_t count = 0;
    if (input == RT_NULL || output == RT_NULL) {
        LOG_E("para invaild\n");
        return RT_ERROR;
    }
    char *ptr = strchr((const char *)input, GPS_SEPARATOR);
    char *ptr_end = strchr((const char *)input, '*');
    if ((ptr == RT_NULL) || (ptr_end == RT_NULL) || ((ptr_end - ptr) > MAX_NMEA_PACK_LEN)) {
        return count;
    }
    char *p1 = NULL;
    char *p2 = NULL;
    while (ptr && ptr < ptr_end && count < section_max_count) {
        p1 = ptr;
        p2 = p1 + 1;
        if ((*p1 == ',') && (*p2 == ',')) {
            output[count++] = p1;
        } else {
            output[count++] = p1 + 1;
        }
        ptr = (char *)strchr(p1 + 1, GPS_SEPARATOR);
    }
    return (count - 1);
}

/* GGA example:
 * $GNGGA,    030432.000,  3112.4500,    N,     12135.1158,    E,
 * <0-$head>,<1-utc_time>,<2-latitude>,<3-N/S>,<4-longtitude>,<5-E/W>,
 * 1,               10,         3.79,      30.9,       M,  8.3,  M,,*70
 * <6-pos_sta>,<7-sat_in_use>,<8-HDOP>,<9-altitude>,<10->,<11->,<12->,<13-CS>
 */
/*******************************************************
*function description: parse GGA
*
*return: FALSE:parse fail,TRUE:parse success
*******************************************************/
bool parse_gga(uint8_t *input, struct NMEA_GGA *pdat)
{
    char *ptr_section[GGA_SECTION_MAX] = { 0 };
    float hdop = 0.00;
    float altitude = 0.00;
    double temp_data;

    if (input == RT_NULL || pdat == NULL) {
        LOG_E("gga para fail\n");
        return false;
    }
    uint8_t count = data_conversion((char *)input, ptr_section, GGA_SECTION_MAX);

    if (PARSE_GGA_MESSAGE_MAX_LEN > count) {
        LOG_E("gga count invalid\n");
        return false;
    }

    temp_data = atof((char *)ptr_section[0]);
    pdat->utc_time = temp_data;

    temp_data = atof((char *)ptr_section[1]);
    pdat->Latitude = latitude_longitude_conversion(temp_data);
    if (*ptr_section[2] == 'N')
        pdat->north_south = 0;
    else if (*ptr_section[2] == 'S')
        pdat->north_south = 1;
    else
        pdat->north_south = -1;

    temp_data = atof(ptr_section[3]);
    pdat->Longitude = latitude_longitude_conversion(temp_data);
    if (*ptr_section[4] == 'E')
        pdat->east_west = 0;
    else if (*ptr_section[4] == 'W')
        pdat->east_west = 1;
    else
        pdat->east_west = -1;

    pdat->sat_in_view = atoi(ptr_section[6]);
    pdat->hdop = atof(ptr_section[7]);
    pdat->altitude = atof(ptr_section[8]);
    return true;
}
/* RMC example:
 * $GNRMC,    033546.000,       A,          3112.4647,   N,     12135.1101,     E,
 * <0-$head>,<1-utc_time>,<2-pos_status>,<3-latitude>,<4-N/S>,<5-longtitude>,<6-E/W>
 *   0.20,      176.33,       120521,   ,,A,V*0F
 * <7-speed>,<8-track_angle>,<9-date>,<10-mag_angle>,<11->,<16-CS>
 */
/*******************************************************
*function description: parse RMC
*
*return: FALSE:parse fail,TRUE:parse success
*******************************************************/
bool parse_rmc(uint8_t *input, struct NMEA_RMC *pdat)
{
    double temp_data;
    float azimuth = 0.00;
    char *ptr_section[RMC_SECTION_MAX] = { 0 };

    if (input == RT_NULL || pdat == NULL) {
        LOG_E("rmc input invalid\n");
        return false;
    }
    uint8_t count = data_conversion((char *)input, ptr_section, RMC_SECTION_MAX);

    if (count < PARSE_RMC_MESSAGE_MAX_LEN) {
        LOG_E("rmc count invalid\n");
        pdat->position_status = -1;
        return false;
    }

    temp_data = atof((char *)ptr_section[0]);
    pdat->utc_time = temp_data;
    if (*ptr_section[1] == 'V')
        pdat->position_status = 0;
    else if (*ptr_section[1] == 'A')
        pdat->position_status = 1;
    else
        pdat->position_status = -1;

    temp_data = atof((char *)ptr_section[2]);
    pdat->Latitude = latitude_longitude_conversion(temp_data);
    if (*ptr_section[3] == 'N')
        pdat->north_south = 0;
    else if (*ptr_section[3] == 'S')
        pdat->north_south = 1;
    else
        pdat->north_south = -1;

    temp_data = atof(ptr_section[4]);
    pdat->Longitude = latitude_longitude_conversion(temp_data);
    if (*ptr_section[5] == 'E')
        pdat->east_west = 0;
    else if (*ptr_section[5] == 'W')
        pdat->east_west = 1;
    else
        pdat->east_west = -1;

    temp_data = atof(ptr_section[6]);
    pdat->speed = temp_data;

    azimuth = atof(ptr_section[7]);
    pdat->azimuth = azimuth;

    temp_data = atof((char *)ptr_section[8]);
    pdat->utc_date = temp_data;

    return true;
}
/* GSV example:
 * $GBGSV,         7,               1,          26,            32,         76,      202,            ,
 * <0-$head>,<1-total_msg>,<2-current_msg>,<3-sat_in_view>,<4-S1_prn>,<5-S1_elev>,<6-S1_amzi>,<7-S1_cnr>
 *     27,          67,     095,            ,           38,         58,         332,        36,
 * <8-S2_prn>,<9-S2_elev>,<10-S2_amzi>,<11-S1_cnr>,<12-S3_prn>,<13-S3_elev>,<14-S3_amzi>,<15-S3_cnr>
 *      03,         53,         201,            ,     1*7B
 * <16-S4_prn>,<17-S4_elev>,<18-S4_amzi>,<19-S4_cnr><20-CS>
 */
/*******************************************************
*function description: parse GSV
*
*return: FALSE:parse fail,TRUE:parse success
*******************************************************/
bool parse_gsv(uint8_t *input, struct NMEA_GSV *pview)
{
    char *ptr_section[GSV_SECTION_MAX] = { 0 };
    int prn; /*prn*/
    int elevation; /*elevation in degrees*/
    int azimuth; /*azimuth in degrees to true*/
    int cnr; /*cnr in dB(dB)*/
    uint8_t count = 0x00, i = 0, total_no = 0, cur_no = 0, cur_index = 0;

    if (input == RT_NULL || pview == NULL) {
        LOG_E("gsv input NULL\n");
        return false;
    }

    count = data_conversion((char *)input, ptr_section, GSV_SECTION_MAX);
    if (count < PARSE_GSV_MESSAGE_MAX_LEN) {
        /*LOG_E("gps_inter_parse_gsv count is small\n");*/
        return false;
    }
    if (ptr_section[0] != NULL) {
        total_no = atoi(ptr_section[0]); /*total gsv msg*/
    } else {
        LOG_E("gsv total_no data invalid\n");
        return false;
    }
    if (ptr_section[1] != NULL) {
        cur_no = atoi(ptr_section[1]); /*current gsv msg no.*/
    } else {
        LOG_E("gsv cur_no data invalid\n");
        return false;
    }
    if (cur_no < 1) {
        LOG_E("currect number is invalid\n");
        return false;
    }
    cur_index = (cur_no - 1) * 4;
    if (cur_no == 1) {
        rt_memset(pview, 0x00, sizeof(struct NMEA_GSV));
    }

    /*update private parse data*/
    if (ptr_section[2] != NULL) {
        /* gps_private_parse_data.sat_in_view = atoi(ptr_section[2]); */
        pview->sates_in_view = atoi(ptr_section[2]);
    } else {
        LOG_E("gsv sat_in_view null\n");
    }

    for (i = 3; i < count && cur_index < GPS_NMEA_MAX_SVVIEW;) {
        if (ptr_section[i] != NULL) {
            prn = atoi(ptr_section[i]);
        } else {
            prn = 0;
        }
        if (ptr_section[i + 1] != NULL) {
            elevation = atoi(ptr_section[i + 1]);
        } else {
            elevation = 0;
        }
        if (ptr_section[i + 2] != NULL) {
            azimuth = atoi(ptr_section[i + 2]);
        } else {
            azimuth = 0;
        }
        if (ptr_section[i + 3] != NULL) {
            cnr = atoi(ptr_section[i + 3]);
        } else {
            cnr = 0;
        }

        pview->rsv[cur_index].prn = prn;
        pview->rsv[cur_index].elevation = elevation;
        pview->rsv[cur_index].azimuth = azimuth;
        pview->rsv[cur_index].cnr = cnr;
        cur_index++;
        i += 4;

        prn = 0;
        elevation = 0;
        azimuth = 0;
        cnr = 0;
    }

    if (total_no < cur_no) {
        LOG_E("gsv total_no >= cur_no\n");
        return false;
    }
    return true;
}

bool parse_epe(uint8_t *input, struct NMEA_EPE *pdat)
{
    if (input == RT_NULL || pdat == NULL) {
        LOG_E("gnepe input NULL\n");
        return false;
    }

    sscanf((char *)input, "$GNEPE,%f,%f*%X", &pdat->hacc, &pdat->vacc, &pdat->checksum);

    return true;
}

#define GPS_MSH_TEST
#ifdef GPS_MSH_TEST
/*gps nmea prase*/
void gps_ospi_parse_nmea(void)
{
    char *rmc = "$GNRMC,014419.00,A,2930.34343,N,10634.36893,E,0.000,354.29,020221,2.60,W,D*37\r\n";
    char *gsv = "$GPGSV,1,1,01,01,,,41,1*61\r\n";
    char *gga = "$GNGGA,014423.00,2930.34347,N,10634.36892,E,2,12,1.00,245.4,M,-26.5,M,,0000*61\r\n";

    rmc += 3;
    if (memcmp(rmc, "RMC", 3) == 0) {
        rmc += 3;
        parse_rmc((uint8_t *)rmc, &g_nmea_parse.rmc);
        rt_kprintf("Latitude: %lf, Longitude: %lf\r\n", g_nmea_parse.rmc.Latitude, g_nmea_parse.rmc.Longitude);
    }

    gsv += 3;
    if (memcmp(gsv, "GSV", 3) == 0) {
        gsv += 3;
        parse_gsv((uint8_t *)gsv, &g_nmea_parse.gsv);
    }

    gga += 3;
    if (memcmp(gga, "GGA", 3) == 0) {
        gga += 3;
        parse_gga((uint8_t *)gga, &g_nmea_parse.gga);
        rt_kprintf("Latitude: %lf, Longitude: %lf\r\n", g_nmea_parse.gga.Latitude, g_nmea_parse.gga.Longitude);
    }
}
MSH_CMD_EXPORT(gps_ospi_parse_nmea, parse gps nmea data);
#endif
