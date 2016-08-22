C/C++的Restful 返回JSON格式的通用架构

测试通过的环境
CentOS-6.5 64bit
(如果不是这个环境，可能需要修改部分的安装和编译脚本)


文件详解:

nginx服务器源码
webserver/nginx-1.9.2.tar.gz

多进程的方式运行(本实例没有使用该方式运行，所有可以忽略这个)
webserver/spawn-fcgi-master.zip

nginx和spawn-fcgi的安装脚本
webserver/install.sh

C++的FastCGI库
lib/cgicc-3.2.16.tar.gz

C的FastCGI库(官方貌似已经没有了,不过通过网上能搜索到)
lib/fcgi-2.4.1.tar.gz

C++的JSON库
lib/jsoncpp-src-0.5.0.tar.gz

所有依赖库的安装脚本
lib/install.sh


使用流程:
1.安装依赖库
在lib目录下执行
sh install.sh

2.安装web server
在webserver目录下执行
sh install.sh

3.配置并运行nginx
默认nginx安装到
/usr/local/nginx下

配置nginx
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


运行nginx
/usr/local/nginx/sbin/nginx


4.编译运行C/C++的web service
直接在webservice目录下执行
make

运行./webservice


5.测试运行情况
浏览器上输入
http://IP地址/test/testget?a=1&b=2&c=3

我们会看到一串打印的JSON数据，至此完毕。


