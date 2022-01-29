## GPS NMEA 解析

- gps 解析文档的整理
- 相关工具的整理
- nmea解析相关

## 前言
- 研究GPS NMEA格式，设计实现一个简洁的NMEA解析组件
- 基于nmealib进行改造

## 重新组织

- 抽取重要的解析函数

```c
rt_err_t nmea_parse_gga(const char *buff, int buff_sz, nmea_gga_t *pack);
rt_err_t nmea_parse_gsa(const char *buff, int buff_sz, nmea_gsa_t *pack);
rt_err_t nmea_parse_gsv(const char *buff, int buff_sz, nmea_gsv_t *pack);
rt_err_t nmea_parse_rmc(const char *buff, int buff_sz, nmea_rmc_t *pack);
rt_err_t nmea_parse_vtg(const char *buff, int buff_sz, nmea_vtg_t *pack);
```
- 解析器可以不使用，用户使用解析函数，自行实现解析

## 测试与验证
- 目前可以在RT-Thread 模拟器上验证，当然可以使用开发板进行实际验证
- `rtt_nmea\bsp\nmea_parse_lib`
- RT-Thread MSH 串口验证效果：GPS数据帧正确的解析
```
msh />nmea_parse_test_rmc
RMC : Latitude: 3115.642200[N], Longitude: 12127.549000[E], Fix: A
msh />
```

## 后续计划
- 使用RT-Thread 消息队列，重构nmealib的队列，理解起来更加容易
- nmealib 使用动态内存申请与释放，这部分是否可以改为静态内存管理
- 根据实际使用不断完善GPS NMEA解析库
