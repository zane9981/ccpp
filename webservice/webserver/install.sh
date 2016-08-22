#!/bin/sh
function make_spawn {
	unzip spawn-fcgi-master.zip
	cd spawn-fcgi-master
	./autogen.sh
	./configure
	make
	make install
    cd -
}

function make_nginx {
	tar vfx nginx-1.9.2.tar.gz
	cd nginx-1.9.2
	./configure
	make
	make install
    cd -
}

if ! spawn-fcgi -v > /dev/null 2>&1 ;then
	make_spawn
fi

if ! /usr/local/nginx/sbin/nginx -v > /dev/null 2>&1; then
	make_nginx
fi
