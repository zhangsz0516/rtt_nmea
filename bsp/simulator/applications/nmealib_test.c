#include <rtthread.h>
#include <nmea/nmea.h>

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

void nmea_parse_test_01(void)
{
    int it;
    nmeaINFO info = { 0 };
    nmeaPARSER parser;
    nmeaPOS dpos;
    double x = 3333333333.444444444444444;

    rt_kprintf("%s : x=%lf\r\n", __func__, x);
    nmea_zero_INFO(&info);
    nmea_parser_init(&parser);

    for (it = 0; it < 8; ++it)
    {
        nmea_parse(&parser, buff[it], (int)rt_strlen(buff[it]), &info);

        nmea_info2pos(&info, &dpos);
        rt_kprintf("%03d, Lat: %lf, Lon: %lf, Sig: %d, Fix: %d\n", it, dpos.lat,
            dpos.lon, info.sig, info.fix);
    }

    nmea_parser_destroy(&parser);
}

MSH_CMD_EXPORT(nmea_parse_test_01, nmea_parse_test_01);

