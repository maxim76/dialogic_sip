BINARY_DIR=./bin

$(BINARY_DIR)/callserver: callserver.cpp udp_request.cpp ssp_scp_interface.hpp ssp_scp_interface.cpp transport_zmq.cpp transport_zmq.hpp channel_manager.hpp sessions.hpp sessions.cpp logging.h logging.cpp
#	RHEL/CentOS6
#	g++ -m32 -L/usr/dialogic/lib -ldl -lm -lgc -o $@ $^ -std=c++0x
#	RHEL/CentOS7 x32
#	g++ -m32 -I/usr/dialogic/inc -L/usr/dialogic/lib -ldl -lm -lgc -o $@  callserver.cpp udp_request.cpp /usr/dialogic/lib/libdxxx.so /usr/dialogic/lib/libsrl.so -std=c++0x
#	RHEL/CentOS7 x64
	g++ -I/usr/dialogic/inc -L/usr/dialogic/lib64 -ldl -lm -lgc64 -lstdc++ -lzmq -o $@ callserver.cpp ssp_scp_interface.cpp udp_request.cpp transport_zmq.cpp sessions.cpp logging.cpp /usr/dialogic/lib64/libdxxdm364.so /usr/dialogic/lib64/libsrl64.so -std=c++11

clean:
	rm -f $(BINARY_DIR)/callserver
