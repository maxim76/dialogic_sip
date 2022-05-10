#include "transport_zmq.hpp"
#include <string>
#include <zmq.hpp>

namespace transport {

class TransportZMQ : public Transport {
public:
	TransportZMQ();
	bool send(TRequestID req_id, TSessionID session_id, char* buf, size_t size) override;
	bool send(TRequestID req_id, TSessionID session_id, const std::string& data) override;
	bool send(TSessionID session_id, char* buf, size_t size) override;
	bool send(TSessionID session_id, const ssp_scp::ISerializable& object) override;
	bool recv(TRequestID* req_id, bool* isTimeout, char* buf, size_t* size) override;
private:
	zmq::context_t context_;
	zmq::socket_t receiver_;
	zmq::socket_t sender_;

	static std::string get_version_str();
};


TransportZMQ::TransportZMQ() : 
	context_(1),
	//receiver_( context_, ZMQ_PULL ),
	//sender_( context_, ZMQ_PUSH )
	receiver_( context_, ZMQ_REP ),
	sender_( context_, ZMQ_REQ )
{
	//fprintf( stdout, "TransportZMQ : ZMQ endpoint initialization\n" );
	fprintf(stdout, "TransportZMQ : ZMQ endpoint initialization. %s\n", TransportZMQ::get_version_str().c_str());

	// Socket to receive messages from SCP
	// TODO: get IP:port prom ctor parameters
	//receiver_.connect( "tcp://localhost:5557" );
	receiver_.bind("tcp://*:5557");

	// Socket to send messages to SCP
	// TODO: get IP:port prom ctor parameters
	//sender_.connect( "tcp://localhost:5558" );
	sender_.bind("tcp://*:5558");
}

bool TransportZMQ::send( TRequestID req_id, TSessionID session_id, char *buf, size_t size ) {
	// TODO: ensure that send does not block
	//zmq::message_t message( buf, size );
	// TODO: избавиться от лишних копирований при передаче итогового буфера
	std::vector<char> payload(sizeof(TRequestID) + sizeof(TSessionID) + size);
	memcpy(&payload[0], &req_id, sizeof(TRequestID));
	memcpy(&payload[sizeof(TRequestID)], &session_id, sizeof(TSessionID));
	memcpy(&payload[sizeof(TRequestID) + sizeof(TSessionID)], buf, size);
	zmq::message_t message(payload.begin(), payload.end());
	sender_.send( message, zmq::send_flags::none );
	return true;
}

bool TransportZMQ::send(TRequestID req_id, TSessionID session_id, const std::string& data) {
	/*
	zmq::message_t message(data.begin(), data.end());
	sender_.send(message, zmq::send_flags::none);
	*/

	// TODO: избавиться от лишних копирований при передаче итогового буфера
	// Пока не получается просто передать zmq::message_t message(data.begin(), data.end()); т.к. надо добавлять еще requireID и sessionID
	std::vector<char> payload(sizeof(TRequestID) + sizeof(TSessionID) + data.size());
	memcpy(&payload[0], &req_id, sizeof(TRequestID));
	memcpy(&payload[sizeof(TRequestID)], &session_id, sizeof(TSessionID));
	memcpy(&payload[sizeof(TRequestID) + sizeof(TSessionID)], data.c_str(), data.size());
	zmq::message_t message(payload.begin(), payload.end());
	sender_.send(message, zmq::send_flags::none);

	return true;
}

bool TransportZMQ::send(TSessionID session_id, char* buf, size_t size) {
	return send(getNextRequestID(), session_id, buf, size);
}

bool TransportZMQ::send(TSessionID session_id, const ssp_scp::ISerializable& object) {
	std::ostringstream ostr;
	object.serialize(ostr);
	// Вероятно, при вызове ostr.str() произойдет лишнее копирование данных во временный объект (который в свою очередь будет тоже скопирован уже в `send`)
	return send(getNextRequestID(), session_id, ostr.str());
}

bool TransportZMQ::recv( TRequestID *req_id, bool *isTimeout, char *buf, size_t *size ) {
	// TODO: обработку таймаутов вынести в Transport
	zmq::pollitem_t items[] = {
			{ sender_, 0, ZMQ_POLLIN, 0 }
	};
	zmq::message_t message;

	if (zmq::poll(&items[0], 1, 0) > 0) {
		if (items[0].revents & ZMQ_POLLIN) {
			zmq::recv_result_t result = sender_.recv(message, zmq::recv_flags::none);
			if (result) {
				printf("TransportZMQ::receive : received RESP %llu bytes\n", *result);
				*size = *result;
				*isTimeout = false;
				// TODO: у message должна быть возможность move
				memcpy(buf, message.data(), message.size());
			}
			else {
				printf("TransportZMQ::receive : error\n");
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
