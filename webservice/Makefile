
CPPFLAGS:= -g

#For fastcig and cgicc
LDFLAGS+= -lfcgi -lcgicc -lfcgi++ 

#For pthread
LDFLAGS+= -lpthread

#For jsoncpp
LDFLAGS+= -ljsoncpp

#For openssl
#LDFLAGS+= -lssl -lcrypto

#for mysql
#LDFLAGS+= -lmysqlclient 

#for debug
LDFLAGS+= -g

#-lboost_system -lcrypto -I /usr/local/ssl/include

all:webservice


webservice:webservice.o FCgiIO.o
	g++ -o $@ $^ ${LDFLAGS}

webservice.o:webservice.cpp
	g++ -o $@ -c $< ${CPPFLAGS}

FCgiIO.o:FCgiIO.cpp FCgiIO.h
	g++ -o $@ -c $< ${CPPFLAGS}

run:
	spawn-fcgi -p 8088 -f ./webservice

debugrun:
	spawn-fcgi -p 8088 -f ./webservice -n

kill:
	killall -9 service

clean:
	\rm -rf *.o webservice
