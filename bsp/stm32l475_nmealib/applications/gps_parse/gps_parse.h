#ifndef __GPS_PARSE__H_
#define __GPS_PARSE__H_

//#include "includes.h"
//#include "pdr_watchApi.h"
//#include "gps_public.h"
#include "convert_time.h"
#include <stdbool.h>
#include <stdint.h>

#define GPS_SEPARATOR             ','
#define GGA_SECTION_MAX           14
#define RMC_SECTION_MAX           12
#define GSV_SECTION_MAX           19
#define GPS_NMEA_MAX_SVVIEW       40 /*12*/
#define GLO_SECTION_MAX           37
#define MDI_GPS_NMEA_MAX_SVVIEW   40 /*12*/
#define MDI_GBD_NMEA_MAX_SVVIEW   38
#define MAX_NMEA_PACK_LEN         200
#define PARSE_GGA_MESSAGE_MAX_LEN 0x08
#define PARSE_GSV_MESSAGE_MAX_LEN 0x07
#define PARSE_RMC_MESSAGE_MAX_LEN 0x09

struct NMEA_GGA {
    double utc_time;
    float Latitude; /*wei du*/
    int8_t north_south;
    float Longitude; /*jing du*/
    int8_t east_west;
    int8_t position_status; /*position status, A:valid, V:invalid*/
    uint8_t sat_in_view;
    float hdop;
    float altitude;
};

struct NMEA_RMC {
    double utc_time;        /*hhmmss.sss*/
    int8_t position_status; /*position status, A:valid, V:invalid*/
    float Latitude;         /*wei du*/
    int8_t north_south;
    float Longitude;        /*jing du*/
    int8_t east_west;
    double speed;
    float azimuth;          /*yaw*/
    double utc_date;        /*DDMMYY*/
};

struct NMEA_EPE {
    float hacc;
    float vacc;
    int checksum;
};

struct NMEA_GSV_SATELITES {
    uint8_t prn; /*prn*/
    uint8_t elevation; /*elevation in degrees*/
    int16_t azimuth; /*azimuth in degrees to true*/
    uint8_t cnr; /*SNR in dB(dB)*/
};
struct NMEA_GSV {
    int16_t sates_in_view; /*satellites in view*/
    struct NMEA_GSV_SATELITES rsv[MDI_GPS_NMEA_MAX_SVVIEW]; /*Satellite attributes*/
};

struct NMEA_ALL_GSV {
    struct NMEA_GSV gps;
    struct NMEA_GSV glo;
    struct NMEA_GSV gal;
    struct NMEA_GSV gbd;
};

struct NMEA_PARSE {
    struct NMEA_GGA gga;
    struct NMEA_RMC rmc;
    struct NMEA_GSV gsv;
    struct NMEA_EPE epe;
};

void gps_clear_private_data(void);
uint8_t data_conversion(char *input, char **output, uint8_t section_max_count);

bool parse_gga(uint8_t *input, struct NMEA_GGA *pdat);
bool parse_rmc(uint8_t *input, struct NMEA_RMC *pdat);
bool parse_gsv(uint8_t *input, struct NMEA_GSV *pdat);
bool parse_epe(uint8_t *input, struct NMEA_EPE *pdat);
#endif
