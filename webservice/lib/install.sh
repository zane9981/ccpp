#!/bin/sh

STATIC="yes"


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

	if [ x$STATIC = x"yes" ] ; then
		g++ *.cpp -I ../../include -c
		ar -r libjsoncpp.a *.o
		cp libjsoncpp.a /usr/lib
	else
		g++ -shared -fPIC *.cpp -I ../../include  -o libjsoncpp.so
		cp libjsoncpp.so /usr/lib
	fi

	cp ../../include/* /usr/include -a
	cd -
}


if [ x$STATIC = x"yes" ] ; then
	if [ ! -e /usr/lib/libcgicc.a ] ; then
		make_cgicc
	fi
		
	if [ ! -e /usr/lib/libfcgi.a ]; then
		make_fcgi
	fi

	if [ ! -e /usr/lib/libjsoncpp.a ];then
		make_jsoncpp
	fi

	if [ -e /usr/lib/libcgicc.so ] ; then
		\rm -f /usr/lib/libcgicc.so
	fi
		
	if [ -e /usr/lib/libfcgi.so ]; then
		\rm -f /usr/lib/libfcgi.so
	fi

	if [ -e /usr/lib/libfcgi++.so ]; then
		\rm -f /usr/lib/libfcgi++.so
	fi

	if [ -e /usr/lib/libjsoncpp.so ];then
		\rm -f /usr/lib/libjsoncpp.so
	fi
else
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
fi

#if ! rpm -ql mysql-devel > /dev/null 2>&1; then
#	yum install -y mysql-devel
#	cp /usr/lib64/mysql/libmysqlclient* /usr/lib
#fi
