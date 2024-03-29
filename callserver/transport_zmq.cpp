#include "transport_zmq.hpp"
#include <string>
#include <zmq.hpp>

#include "logging.h"

namespace transport {

class TransportZMQ : public Transport {
public:
	TransportZMQ();
	bool send(TSessionID session_id, const ssp_scp::ISerializable& object) override;
	bool recv(TRequestID* req_id, bool* isTimeout, char* buf, size_t* size) override;
private:
	zmq::context_t context_;
	zmq::socket_t receiver_;
	zmq::socket_t sender_;

	static std::string get_version_str();
	void sendStringBuf(const std::string& data);
};


TransportZMQ::TransportZMQ() : 
	context_(1),
	//receiver_( context_, ZMQ_PULL ),
	//sender_( context_, ZMQ_PUSH )
	receiver_( context_, ZMQ_REP ),
	sender_( context_, ZMQ_REQ )
{
	Log(TRC_NET, TRC_INFO, -1, "TransportZMQ : ZMQ endpoint initialization. %s", TransportZMQ::get_version_str().c_str());

	// Socket to receive messages from SCP
	// TODO: get IP:port prom ctor parameters
	//receiver_.connect( "tcp://localhost:5557" );
	receiver_.bind("tcp://*:5557");

	// Socket to send messages to SCP
	// TODO: get IP:port prom ctor parameters
	//sender_.connect( "tcp://localhost:5558" );
	sender_.bind("tcp://*:5558");
}

bool TransportZMQ::send(TSessionID session_id, const ssp_scp::ISerializable& object) {
	std::ostringstream ostr;
	
	const TRequestID req_id = getNextRequestID();
	ostr.write(reinterpret_cast<const char*>(&req_id), sizeof(TRequestID));
	ostr.write(reinterpret_cast<const char*>(&session_id), sizeof(TSessionID));
	object.serialize(ostr);

	sendStringBuf(ostr.str());
	return true;
}

void TransportZMQ::sendStringBuf(const std::string& data) {
	zmq::message_t message(data.begin(), data.end());

	std::string data_dump;
	data_dump.resize(data.size() * 3);
	for (size_t i = 0; i < data.size(); ++i) {
		unsigned char digit = *((char*)data.data() + i);
		sprintf(&(data_dump[i * 3]), "%02x ", digit);
	}

	Log(TRC_NET, TRC_DUMP, -1, "TransportZMQ::sendStringBuf : sending data (%llu bytes) : [%s]", data.size(), data_dump.c_str());
	sender_.send(message, zmq::send_flags::none);
}

bool TransportZMQ::recv(TSessionID* session_id, bool* isTimeout, char* buf, size_t* size) {
	// TODO: обработку таймаутов вынести в Transport
	zmq::pollitem_t items[] = {
			{ sender_, 0, ZMQ_POLLIN, 0 }
	};
	zmq::message_t message;

	if (zmq::poll(&items[0], 1, 0) > 0) {
		if (items[0].revents & ZMQ_POLLIN) {
			zmq::recv_result_t maybe_size = sender_.recv(message, zmq::recv_flags::none);
			if (maybe_size) {
				auto received_size = *maybe_size;

				std::string data_dump;
				data_dump.resize(received_size * 3);
				for (size_t i = 0; i < received_size; ++i) {
					unsigned char digit = *((char*)message.data() + i);
					sprintf(&(data_dump[i*3]), "%02x ", digit);
				}
				Log(TRC_NET, TRC_DUMP, -1, "TransportZMQ::receive : received RESP %llu bytes : [%s]", received_size, data_dump.c_str());

				if (received_size < sizeof(TRequestID) + sizeof(TSessionID) + sizeof(TSCPCommandCode)) {
					Log(TRC_NET, TRC_ERROR, -1, "TransportZMQ::receive : too short data chunk received");
					return false;
				}
				*isTimeout = false;
				// TODO: у message должна быть возможность move ?
				TRequestID req_id;
				memcpy((char*)&req_id, message.data(), sizeof(TRequestID));
				memcpy((char*)session_id, (char *)message.data() + sizeof(TRequestID), sizeof(TSessionID));
				auto data_size = received_size - sizeof(TRequestID) - sizeof(TSessionID);
				memcpy(buf, (char *)message.data() + sizeof(TRequestID) + sizeof(TSessionID), data_size);
				*size = data_size;
			}
			else {
				Log(TRC_NET, TRC_ERROR, -1, "TransportZMQ::receive : error\n");
				return false;
			}

		}
		return true;
	}
	return false;
}


std::string TransportZMQ::get_version_str() {
	int major = 0;
	int minor = 0;
	int patch = 0;
	zmq_version(&major, &minor, &patch);
	return "Current 0MQ version is : " + std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
}

std::unique_ptr<Transport> createTransportZMQ() {
	return std::unique_ptr<Transport>( new TransportZMQ() );
}

} // namespace transport
