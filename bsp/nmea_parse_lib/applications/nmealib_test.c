#include <rtthread.h>
#include <stdio.h>

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

#define USING_NMEA_LIB

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

#else

#include <nmea_parse.h>

const char *buff[] =
{
        "$GPRMC,173843,A,3349.896,N,11808.521,W,000.0,360.0,230108,013.4,E*69\r\n",
        "$GPGGA,111609.14,5001.27,N,3613.06,E,3,08,0.0,10.2,M,0.0,M,0.0,0000*70\r\n",
        "$GPGSV,2,1,08,01,05,005,80,02,05,050,80,03,05,095,80,04,05,140,80*7f\r\n",
        "$GPGSV,2,2,08,05,05,185,80,06,05,230,80,07,05,275,80,08,05,320,80*71\r\n",
        "$GPGSA,A,3,01,02,03,04,05,06,07,08,00,00,00,00,0.0,0.0,0.0*3a\r\n",
        "$GPRMC,111609.14,A,5001.27,N,3613.06,E,11.2,0.0,261206,0.0,E*50\r\n",
        "$GPVTG,217.5,T,208.8,M,000.00,N,000.01,K*4C\r\n",
        "$GPRMC,031024.000,A,3115.6422,N,12127.5490,E,0.58,98.86,180918,,,A*5A\r\n"
};

const char *buff2 = "$GPRMC,031024.000,A,3115.6422,N,12127.5490,E,0.58,98.86,180918,,,A*5A\r\n";

void nmea_parse_test_01(void)
{
    int it;
    nmea_info_t info = { 0 };
    nmea_parser_t parser;

    double x = 3333333333.444444444444444;

    dbg_printf("%s : x=%lf\r\n", __func__, x);
    nmea_parser_init(&parser);

#if 1
    for (it = 0; it < 8; ++it)
    {
        nmea_parse(&parser, buff[it], (int)rt_strlen(buff[it]), &info);

        dbg_printf("RMC : Lat: %lf, Lon: %lf, Sig: %d, Fix: %d\n", info.lat,
            info.lon, info.sig, info.fix);
    }
#endif
    int len = rt_strlen(buff2);
    dbg_printf("buffer len = %d\n", len);
    nmea_parse(&parser, buff2, (int)rt_strlen(buff2), &info);

    dbg_printf("RMC : Lat: %lf, Lon: %lf, Sig: %d, Fix: %d\n", info.lat,
        info.lon, info.sig, info.fix);

    nmea_parser_destroy(&parser);
}

MSH_CMD_EXPORT(nmea_parse_test_01, nmea_parse_test_01);

#endif

