#include <rtthread.h>
#include <stdio.h>

#define USING_NMEA_LIB

#define DBG_BUFF_MAX_LEN          256

/* debug print */
int dbg_printf(const char *fmt, ...)
{
    va_list args;
    static char rt_log_buf[DBG_BUFF_MAX_LEN] = { 0 };

    va_start(args, fmt);
    int length = vsnprintf(rt_log_buf, sizeof(rt_log_buf) - 1, fmt, args);

    rt_kputs(rt_log_buf);

    return length;
}

#ifdef USING_NMEA_LIB

#include <nmea_parse.h>
const char *rmc_buf = "$GPRMC,031024.000,A,3115.6422,N,12127.5490,E,0.58,98.86,180918,,,A*5A\r\n";
const char *gga_buf = "$GPGGA,082006.000,3852.9276,N,11527.4283,E,1,08,1.0,20.6,M,,,,0000*35\r\n";
const char *vtg_buf = "$GPVTG,0.0,T,,M,0.00,N,0.00,K,N*50\r\n";
const char *gsa_buf = "$GPGSA,A,3,01,20,19,13,,,,,,,,,40.4,24.4,32.2*0A\r\n";
const char *gsv_buf = "$GPGSV,4,1,13,28,86,324,,03,46,082,22,17,46,326,19,06,31,244,37*7D\r\n";

void nmea_parse_test_rmc(void)
{
    nmea_rmc_t pack = { 0 };

    nmea_parse_rmc(rmc_buf, rt_strlen(rmc_buf), &pack);
    dbg_printf("RMC : Latitude: %lf[%c], Longitude: %lf[%c], Fix: %c\n", pack.lat, pack.ns,
        pack.lon, pack.ew, pack.status);
}
MSH_CMD_EXPORT(nmea_parse_test_rmc, nmea_parse_test_rmc);

void nmea_parse_test_gga(void)
{
    nmea_gga_t pack = { 0 };

    nmea_parse_gga(gga_buf, rt_strlen(gga_buf), &pack);
    dbg_printf("GGA : Latitude: %lf[%c], Longitude: %lf[%c], sig: %d\n", pack.lat, pack.ns,
        pack.lon, pack.ew, pack.sig);
}
MSH_CMD_EXPORT(nmea_parse_test_gga, nmea_parse_test_gga);

void nmea_parse_test_vtg(void)
{
    nmea_vtg_t pack = { 0 };

    nmea_parse_vtg(vtg_buf, rt_strlen(vtg_buf), &pack);
    dbg_printf("VTG : dir: %lf, Magnetic: %lf, speed knots: %lf, speed kilometers: %lf,\n",
        pack.dir, pack.dec, pack.spn, pack.spk);
    dbg_printf("VTG : degrees True: %c, degrees Magnetic: %c, speed_knots: %c, speed_km_per_h: %c,\n",
        pack.dir_t, pack.dec_m, pack.spn_n, pack.spk_k);
}
MSH_CMD_EXPORT(nmea_parse_test_vtg, nmea_parse_test_vtg);

void nmea_parse_test_gsa(void)
{
    nmea_gsa_t pack = { 0 };

    nmea_parse_gsa(gsa_buf, rt_strlen(gsa_buf), &pack);
    dbg_printf("GSA : fix_mode: %c, fix_type: %d\n", pack.fix_mode, pack.fix_type);
}
MSH_CMD_EXPORT(nmea_parse_test_gsa, nmea_parse_test_gsa);

void nmea_parse_test_gsv(void)
{
    nmea_gsv_t pack = { 0 };

    nmea_parse_gsv(gsv_buf, rt_strlen(gsv_buf), &pack);
    dbg_printf("GSV : pack_count: %d, pack_index: %d, sat_count : %d\n", pack.pack_count,
        pack.pack_index, pack.sat_count);
}
MSH_CMD_EXPORT(nmea_parse_test_gsv, nmea_parse_test_gsv);

#endif
