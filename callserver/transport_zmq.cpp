#include "transport_zmq.hpp"
#include <zmq.hpp>

namespace transport {

class TransportZMQ : public Transport {
public:
	TransportZMQ();
	bool send( TRequestID req_id, char *buf, size_t size ) override;
	bool receive( TRequestID *req_id, bool *isTimeout, char *buf, size_t *size ) override;
private:
	zmq::context_t context_;
	zmq::socket_t receiver_;
	zmq::socket_t sender_;
};


TransportZMQ::TransportZMQ() : 
	context_(1),
	receiver_( context_, ZMQ_PULL ),
	sender_( context_, ZMQ_PUSH )
{
	//zmq::context_t context( 1 );
	//zmq::socket_t socket( context, zmq::socket_type::req );
	fprintf( stdout, "TransportZMQ : ZMQ endpoint initialization\n" );

	// Socket to receive messages from SCP
	//zmq::socket_t receiver( context, ZMQ_PULL );
	// TODO: get IP:port prom ctor parameters
	receiver_.connect( "tcp://localhost:5557" );

	// Socket to send messages to SCP
	//zmq::socket_t sender( context, ZMQ_PUSH );
	// TODO: get IP:port prom ctor parameters
	sender_.connect( "tcp://localhost:5558" );
}

bool TransportZMQ::send( TRequestID req_id, char *buf, size_t size ) {
	//  Initialize poll set
	/*
	zmq::pollitem_t items[] = {
		{ receiver_, 0, ZMQ_POLLIN, 0 },
		{ sender_, 0, ZMQ_POLLOUT, 0 }
	};
	*/
	//zmq::message_t message;
	//memcpy( message.data(), "Test", 4 );
	zmq::message_t message( buf, size );
	sender_.send( message, zmq::send_flags::none );
	return true;
}

bool TransportZMQ::receive( TRequestID *req_id, bool *isTimeout, char *buf, size_t *size ) {
	return true;
}

std::unique_ptr<Transport> createTransportZMQ() {
	return std::unique_ptr<Transport>( new TransportZMQ() );
}

} // namespace transport
