C/C++��Restful ����JSON��ʽ��ͨ�üܹ�

����ͨ���Ļ���
CentOS-6.5 64bit
(����������������������Ҫ�޸Ĳ��ֵİ�װ�ͱ���ű�)


�ļ����:

nginx������Դ��
webserver/nginx-1.9.2.tar.gz

����̵ķ�ʽ����(��ʵ��û��ʹ�ø÷�ʽ���У����п��Ժ������)
webserver/spawn-fcgi-master.zip

nginx��spawn-fcgi�İ�װ�ű�
webserver/install.sh

C++��FastCGI��
lib/cgicc-3.2.16.tar.gz

C��FastCGI��(�ٷ�ò���Ѿ�û����,����ͨ��������������)
lib/fcgi-2.4.1.tar.gz

C++��JSON��
lib/jsoncpp-src-0.5.0.tar.gz

����������İ�װ�ű�
lib/install.sh


ʹ������:
1.��װ������
��libĿ¼��ִ��
sh install.sh

2.��װweb server
��webserverĿ¼��ִ��
sh install.sh

3.���ò�����nginx
Ĭ��nginx��װ��
/usr/local/nginx��

����nginx
vim /usr/local/nginx/conf/nginx.conf

worker_processes  auto;
events {
    worker_connections  20480;
    use epoll;
}
http {
    include       mime.types;
    default_type  application/octet-stream;
    add_header Access-Control-Allow-Origin *;
    sendfile        on;
    keepalive_timeout  65;
    server_tokens off;
    server {
        listen       80;
        server_name  localhost;
        location / {
            root html;
            index  index.html index.htm;
        }
        location ^~ /test/ {
                fastcgi_pass webserver;
                fastcgi_index index.cgi;
                include fastcgi.conf;
        }
        error_page   500 502 503 504  /50x.html;
        location = /50x.html {
            root   html;
        }
    }
    upstream webserver {
        server 127.0.0.1:8088 max_fails=3 fail_timeout=30s;
    }
}


����nginx
/usr/local/nginx/sbin/nginx


4.��������C/C++��web service
ֱ����webserviceĿ¼��ִ��
make

����./webservice


5.�����������
�����������
http://IP��ַ/test/testget?a=1&b=2&c=3

���ǻῴ��һ����ӡ��JSON���ݣ�������ϡ�


