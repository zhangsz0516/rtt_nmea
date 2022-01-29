#ifndef __NMEA_PARSE_H__
#define __NMEA_PARSE_H__

#include <rtthread.h>

#ifdef  __cplusplus
extern "C" {
#endif

#define NMEA_SIG_BAD            (0)
#define NMEA_SIG_LOW            (1)
#define NMEA_SIG_MID            (2)
#define NMEA_SIG_HIGH           (3)

#define NMEA_FIX_BAD            (1)
#define NMEA_FIX_2D             (2)
#define NMEA_FIX_3D             (3)

#define NMEA_MAXSAT             (12)
#define NMEA_SATINPACK          (4)
#define NMEA_NSATPACKS          (NMEA_MAXSAT / NMEA_SATINPACK)

#define NMEA_CONVSTR_BUF        (256)
#define NMEA_TIMEPARSE_BUF      (256)

#define NMEA_DEF_PARSEBUFF      (1024)
#define NMEA_MIN_PARSEBUFF      (256)

#define NMEA_TUD_KNOTS          (1.852) /**< Knots, kilometer / NMEA_TUD_KNOTS = knot */

#define NMEA_ASSERT             RT_ASSERT
#define nmea_error              rt_kprintf

/**
 * Date and time data
 * @see nmea_time_now
 */
typedef struct _nmea_time
{
    int year; /**< Years since 1900 */
    int mon; /**< Months since January - [0,11] */
    int day; /**< Day of the month - [1,31] */
    int hour; /**< Hours since midnight - [0,23] */
    int min; /**< Minutes after the hour - [0,59] */
    int sec; /**< Seconds after the minute - [0,59] */
    int hsec; /**< Hundredth part of second - [0,99] */
} nmea_time_t;

/**
 * Information about satellite
 * @see nmeaSATINFO
 * @see nmea_gsv_t
 */
typedef struct _nmea_satellite
{
    int     id;         /**< Satellite PRN number */
    int     in_use;     /**< Used in position fix */
    int     elv;        /**< Elevation in degrees, 90 maximum */
    int     azimuth;    /**< Azimuth, degrees from true north, 000 to 359 */
    int     sig;        /**< Signal, 00-99 dB */
} nmea_satellite_t;

/**
 * Information about all satellites in view
 * @see nmea_info_t
 * @see nmea_gsv_t
 */
typedef struct _nmea_sta_info
{
    int     inuse; /**< Number of satellites in use (not those in view) */
    int     inview; /**< Total number of satellites in view */
    nmea_satellite_t sat[NMEA_MAXSAT]; /**< Satellites information */
} nmea_sat_info_t;

/**
 * Summary GPS information from all parsed packets,
 * used also for generating NMEA stream
 * @see nmea_parse
 * @see nmea_gga_info,  nmea_...info
 */
typedef struct _nmea_info
{
    int     smask; /**< Mask specifying types of packages from which data have been obtained */

    nmea_time_t utc; /**< UTC of position */

    int     sig; /**< GPS quality indicator (0 = Invalid; 1 = Fix; 2 = Differential, 3 = Sensitive) */
    int     fix; /**< Operating mode, used for navigation (1 = Fix not available; 2 = 2D; 3 = 3D) */

    double  PDOP; /**< Position Dilution Of Precision */
    double  HDOP; /**< Horizontal Dilution Of Precision */
    double  VDOP; /**< Vertical Dilution Of Precision */

    double  lat; /**< Latitude in NDEG - +/-[degree][min].[sec/60] */
    double  lon; /**< Longitude in NDEG - +/-[degree][min].[sec/60] */
    double  elv; /**< Antenna altitude above/below mean sea level (geoid) in meters */
    double  speed; /**< Speed over the ground in kilometers/hour */
    double  direction; /**< Track angle in degrees True */
    double  declination; /**< Magnetic variation degrees (Easterly var. subtracts from true course) */

    nmea_sat_info_t satinfo; /**< Satellites information */
} nmea_info_t;

typedef struct _nmea_parser
{
    void *top_node;
    void *end_node;
    unsigned char *buffer;
    int buff_size;
    int buff_use;
} nmea_parser_t;

/**
 * NMEA packets type which parsed and generated by library
 */
enum NMEA_PACK_TYPE
{
    GPNON = 0x0000,   /**< Unknown packet type. */
    GPGGA = 0x0001,   /**< GGA - Essential fix data which provide 3D location and accuracy data. */
    GPGSA = 0x0002,   /**< GSA - GPS receiver operating mode, SVs used for navigation, and DOP values. */
    GPGSV = 0x0004,   /**< GSV - Number of SVs in view, PRN numbers, elevation, azimuth & SNR values. */
    GPRMC = 0x0008,   /**< RMC - Recommended Minimum Specific GPS/TRANSIT Data. */
    GPVTG = 0x0010    /**< VTG - Actual track made good and speed over ground. */
};

/**
 * GGA packet information structure (Global Positioning System Fix Data)
 */
typedef struct _nmea_gga
{
    nmea_time_t utc;       /**< UTC of position (just time) */
    double  lat;        /**< Latitude in NDEG - [degree][min].[sec/60] */
    char    ns;         /**< [N]orth or [S]outh */
    double  lon;        /**< Longitude in NDEG - [degree][min].[sec/60] */
    char    ew;         /**< [E]ast or [W]est */
    int     sig;        /**< GPS quality indicator (0 = Invalid; 1 = Fix; 2 = Differential, 3 = Sensitive) */
    int     satinuse;   /**< Number of satellites in use (not those in view) */
    double  HDOP;       /**< Horizontal dilution of precision */
    double  elv;        /**< Antenna altitude above/below mean sea level (geoid) */
    char    elv_units;  /**< [M]eters (Antenna height unit) */
    double  diff;       /**< Geoidal separation (Diff. between WGS-84 earth ellipsoid and mean sea level. '-' = geoid is below WGS-84 ellipsoid) */
    char    diff_units; /**< [M]eters (Units of geoidal separation) */
    double  dgps_age;   /**< Time in seconds since last DGPS update */
    int     dgps_sid;   /**< DGPS station ID number */
} nmea_gga_t;

/**
 * GSA packet information structure (Satellite status)
 */
typedef struct _nmea_gsa
{
    char    fix_mode;   /**< Mode (M = Manual, forced to operate in 2D or 3D; A = Automatic, 3D/2D) */
    int     fix_type;   /**< Type, used for navigation (1 = Fix not available; 2 = 2D; 3 = 3D) */
    int     sat_prn[NMEA_MAXSAT]; /**< PRNs of satellites used in position fix (null for unused fields) */
    double  PDOP;       /**< Dilution of precision */
    double  HDOP;       /**< Horizontal dilution of precision */
    double  VDOP;       /**< Vertical dilution of precision */
} nmea_gsa_t;

/**
 * GSV packet information structure (Satellites in view)
 */
typedef struct _nmea_gsv
{
    int     pack_count; /**< Total number of messages of this type in this cycle */
    int     pack_index; /**< Message number */
    int     sat_count;  /**< Total number of satellites in view */
    nmea_satellite_t sat_data[NMEA_SATINPACK];
} nmea_gsv_t;

/**
 * RMC packet information structure (Recommended Minimum sentence C)
 */
typedef struct _nmea_rmc
{
    nmea_time_t utc;       /**< UTC of position */
    char    status;     /**< Status (A = active or V = void) */
    double  lat;        /**< Latitude in NDEG - [degree][min].[sec/60] */
    char    ns;         /**< [N]orth or [S]outh */
    double  lon;        /**< Longitude in NDEG - [degree][min].[sec/60] */
    char    ew;         /**< [E]ast or [W]est */
    double  speed;      /**< Speed over the ground in knots */
    double  direction;  /**< Track angle in degrees True */
    double  declination; /**< Magnetic variation degrees (Easterly var. subtracts from true course) */
    char    declin_ew;  /**< [E]ast or [W]est */
    char    mode;       /**< Mode indicator of fix type (A = autonomous, D = differential, E = estimated, N = not valid, S = simulator) */
} nmea_rmc_t;

/**
 * VTG packet information structure (Track made good and ground speed)
 */
typedef struct _nmea_vtg
{
    double  dir;        /**< True track made good (degrees) */
    char    dir_t;      /**< Fixed text 'T' indicates that track made good is relative to true north */
    double  dec;        /**< Magnetic track made good */
    char    dec_m;      /**< Fixed text 'M' */
    double  spn;        /**< Ground speed, knots */
    char    spn_n;      /**< Fixed text 'N' indicates that speed over ground is in knots */
    double  spk;        /**< Ground speed, kilometers per hour */
    char    spk_k;      /**< Fixed text 'K' indicates that speed over ground is in kilometers/hour */
} nmea_vtg_t;

int nmea_scanf(const char *buff, int buff_sz, const char *format, ...);
int nmea_atoi(const char *str, int str_sz, int radix);

int     nmea_parser_init(nmea_parser_t *parser);
void    nmea_parser_destroy(nmea_parser_t *parser);

int     nmea_parse(
    nmea_parser_t *parser,
    const char *buff, int buff_sz,
    nmea_info_t *info
);

int nmea_parse_gga(const char *buff, int buff_sz, nmea_gga_t *pack);
int nmea_parse_gsa(const char *buff, int buff_sz, nmea_gsa_t *pack);
int nmea_parse_gsv(const char *buff, int buff_sz, nmea_gsv_t *pack);
int nmea_parse_rmc(const char *buff, int buff_sz, nmea_rmc_t *pack);
int nmea_parse_vtg(const char *buff, int buff_sz, nmea_vtg_t *pack);

#ifdef  __cplusplus
}
#endif

#endif