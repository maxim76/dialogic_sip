#pragma once
#include <cstddef>
#include "ssp_scp_interface.hpp"
#include "sessions.hpp"

namespace transport {

typedef uint32_t TRequestID;

class Transport {
public:
	virtual ~Transport() {};
	virtual bool send(TSessionID session_id, const ssp_scp::ISerializable& object) = 0;
	//virtual bool recv(TRequestID* req_id, bool* isTimeout, char* buf, size_t* size) = 0;
	virtual bool recv(TSessionID* session_id, bool* isTimeout, char* buf, size_t* size) = 0;

	TRequestID getNextRequestID() {
		return currentRequestID++;
	}
private:
	TRequestID currentRequestID = 0;
};

} // namespace transport
