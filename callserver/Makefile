BINARY_DIR=./bin

$(BINARY_DIR)/callserver: callserver.cpp udp_request.cpp ssp_scp_interface.hpp
#	RHEL/CentOS6
#	g++ -m32 -L/usr/dialogic/lib -ldl -lm -lgc -o $@ $^ -std=c++0x
#	RHEL/CentOS7
	g++ -m32 -I/usr/dialogic/inc -L/usr/dialogic/lib -ldl -lm -lgc -o $@  callserver.cpp udp_request.cpp /usr/dialogic/lib/libdxxx.so /usr/dialogic/lib/libsrl.so -std=c++0x

clean:
	rm -f $(BINARY_DIR)/callserver
