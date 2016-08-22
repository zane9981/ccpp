#!/bin/sh

function make_cgicc {
	tar vfx cgicc-3.2.16.tar.gz
	cd cgicc-3.2.16
	./configure
	make
	make install
	cd -
}

function make_fcgi {
	tar vfx fcgi-2.4.1.tar.gz
	cd fcgi-2.4.1
	./configure
	make
	make install
	cp /usr/local/lib/libfcgi* /usr/lib/
	cd -
}

function make_jsoncpp {
	tar vfx jsoncpp-src-0.5.0.tar.gz
	cd jsoncpp-src-0.5.0/src/lib_json
	g++ -shared -fPIC *.cpp -I ../../include -o libjsoncpp.so
	cp libjsoncpp.so /usr/lib
	cp ../../include/* /usr/include -a
	cd -
}

if [ ! -e /usr/lib/libcgicc.so ] ; then
	make_cgicc
fi
	
if [ ! -e /usr/lib/libfcgi.so ]; then
	make_fcgi
fi

if [ ! -e /usr/lib/libjsoncpp.so ];then
	make_jsoncpp
fi

ldconfig

#if ! rpm -ql mysql-devel > /dev/null 2>&1; then
#	yum install -y mysql-devel
#	cp /usr/lib64/mysql/libmysqlclient* /usr/lib
#fi
