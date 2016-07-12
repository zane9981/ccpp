描述:RTPC信令层压力测试小工具

作者:huangzheng

创建日期:2016/07/12



编译:
在Linux环境下,直接make生成rtpc_stress和rtpc_stress2



说明:
rtpc_stress是第一套的测试程序，测试的流程如下
1.发送指定数量的O命令
2.等待一个指定的时长
3.发送指定数量的D命令
4.等待一个指定的时长
5.统计接收O命令和D命令的反馈信息

rptc_stress2是第二套的测试程序，测试的流程如下
1.发送指定数量的O命令
2.当O命令到一定数量的时候，发送对应的D命令
3.这个时候O命令和D命令一起都有发送，但是O命令的并发量会保持稳定在一个范围
4.循环2-3这两个流程，直到循环次数达到一个指定的数量
5.统计接收O命令和D命令的反馈信息

建议使用第二套程序.



使用:
第一套程序
Usage:./rtpc_stress <rtpc ip>[:rtpc port] <-l rtpp left> [-r rtpp right] [-c count] [-i interval] [-t one second times]

<rtpc ip>             指定的rtpc的IP地址，必填
[:rtpc prot]          指定的rptc的端口，默认9966
<-l rtpp left>        指定的第一个的rtpp的地址，必填
[-r rtpp left]        指定的第二个的rtpp的地址，可不填，默认只测试一个rtpp
[-c count]            指定O命令的发送总数，默认10
[-i interval]         指定发送完所有的O命令和D命令后等待的时间长度，默认5000ms，单位是毫秒
[-t one second times] 指定一秒钟内发送O命令的次数，默认100。


第二套程序
Usage:./rtpc_stress2 <rtpc ip>[:rtpc port]
<-l rtpp left>
[-r rtpp right]
[-c count]
[-i interval(millisecond)]
[-t one second times]
[-m max concurrence]

<rtpc ip>             指定的rtpc的IP地址，必填
[:rtpc prot]          指定的rptc的端口，默认9966
<-l rtpp left>        指定的第一个的rtpp的地址，必填
[-r rtpp left]        指定的第二个的rtpp的地址，可不填，默认只测试一个rtpp
[-c count]            指定O命令的发送总数，默认10
[-i interval]         指定发送完所有的O命令和D命令后等待的时间长度，默认5000ms，单位是毫秒
[-t one second times] 指定一秒钟内发送O命令的次数，默认100。
[-m max concurrence]  O命令发送的最大并发量，默认4000