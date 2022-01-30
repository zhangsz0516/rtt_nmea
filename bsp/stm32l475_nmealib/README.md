## STM32L475 Pandora 开发板

- RT-Thread 基础组件的评估与设计

- GPS nmealib组件的解析验证

```c
msh >
 \ | /
- RT -     Thread Operating System
 / | \     4.1.0 build Jan 30 2022 09:41:13
 2006 - 2021 Copyright by rt-thread team
msh >
msh >n
nmea_parse_test_rmc
nmea_parse_test_gga
nmea_parse_test_vtg
nmea_parse_test_gsa
nmea_parse_test_gsv

msh >nmea_parse_test_rmc
RMC : Latitude: 3115.642200[N], Longitude: 12127.549000[E], Fix: A

msh >nmea_parse_test_gga
GGA : Latitude: 3852.927600[N], Longitude: 11527.428300[E], sig: 1

msh >nmea_parse_test_vtg
VTG : dir: 0.000000, Magnetic: 0.000000, speed knots: 0.000000, speed kilometers: 0.000000,
VTG : degrees True: T, degrees Magnetic: M, speed_knots: N, speed_km_per_h: K,

msh >nmea_parse_test_gsa
GSA : fix_mode: A, fix_type: 3

msh >nmea_parse_test_gsv
GSV : pack_count: 4, pack_index: 1, sat_count : 13

```
