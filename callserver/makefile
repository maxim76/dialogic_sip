callserver: callserver.o udp_request.o
#	RHEL/CentOS6
#	g++ -m32 -L/usr/dialogic/lib -ldl -lm -lgc -o $@ $^ -std=c++0x
#	RHEL/CentOS7
	g++ -m32 -L/usr/dialogic/lib -ldl -lm -lgc -o $@ $^ /usr/dialogic/lib/libdxxx.so /usr/dialogic/lib/libsrl.so -std=c++0x

callserver.o: callserver.cpp
	g++ -m32 -g -Wall -c -I/usr/dialogic/inc -Dlint -o $@ $^ -std=c++0x

udp_request.o: udp_request.cpp
	g++ -m32 -g -c -o $@ $^ -std=c++0x

clean:
	rm -f *.o callserver
