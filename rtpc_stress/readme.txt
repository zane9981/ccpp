����:RTPC�����ѹ������С����

����:huangzheng

��������:2016/07/12



����:
��Linux������,ֱ��make����rtpc_stress��rtpc_stress2



˵��:
rtpc_stress�ǵ�һ�׵Ĳ��Գ��򣬲��Ե���������
1.����ָ��������O����
2.�ȴ�һ��ָ����ʱ��
3.����ָ��������D����
4.�ȴ�һ��ָ����ʱ��
5.ͳ�ƽ���O�����D����ķ�����Ϣ

rptc_stress2�ǵڶ��׵Ĳ��Գ��򣬲��Ե���������
1.����ָ��������O����
2.��O���һ��������ʱ�򣬷��Ͷ�Ӧ��D����
3.���ʱ��O�����D����һ���з��ͣ�����O����Ĳ������ᱣ���ȶ���һ����Χ
4.ѭ��2-3���������̣�ֱ��ѭ�������ﵽһ��ָ��������
5.ͳ�ƽ���O�����D����ķ�����Ϣ

����ʹ�õڶ��׳���.



ʹ��:
��һ�׳���
Usage:./rtpc_stress <rtpc ip>[:rtpc port] <-l rtpp left> [-r rtpp right] [-c count] [-i interval] [-t one second times]

<rtpc ip>             ָ����rtpc��IP��ַ������
[:rtpc prot]          ָ����rptc�Ķ˿ڣ�Ĭ��9966
<-l rtpp left>        ָ���ĵ�һ����rtpp�ĵ�ַ������
[-r rtpp left]        ָ���ĵڶ�����rtpp�ĵ�ַ���ɲ��Ĭ��ֻ����һ��rtpp
[-c count]            ָ��O����ķ���������Ĭ��10
[-i interval]         ָ�����������е�O�����D�����ȴ���ʱ�䳤�ȣ�Ĭ��5000ms����λ�Ǻ���
[-t one second times] ָ��һ�����ڷ���O����Ĵ�����Ĭ��100��


�ڶ��׳���
Usage:./rtpc_stress2 <rtpc ip>[:rtpc port]
<-l rtpp left>
[-r rtpp right]
[-c count]
[-i interval(millisecond)]
[-t one second times]
[-m max concurrence]

<rtpc ip>             ָ����rtpc��IP��ַ������
[:rtpc prot]          ָ����rptc�Ķ˿ڣ�Ĭ��9966
<-l rtpp left>        ָ���ĵ�һ����rtpp�ĵ�ַ������
[-r rtpp left]        ָ���ĵڶ�����rtpp�ĵ�ַ���ɲ��Ĭ��ֻ����һ��rtpp
[-c count]            ָ��O����ķ���������Ĭ��10
[-i interval]         ָ�����������е�O�����D�����ȴ���ʱ�䳤�ȣ�Ĭ��5000ms����λ�Ǻ���
[-t one second times] ָ��һ�����ڷ���O����Ĵ�����Ĭ��100��
[-m max concurrence]  O����͵���󲢷�����Ĭ��4000