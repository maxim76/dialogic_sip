#pragma once
#include <cstddef>

namespace transport {

typedef uint32_t TRequestID;
typedef uint32_t TSessionID;

class Transport {
public:
	virtual ~Transport() {};
	// TODO: передавать буфер как std::string& чтоб обойтись одним параметром?
	virtual bool send(TRequestID req_id, TSessionID session_id, char* buf, size_t size) = 0;
	virtual bool send(TSessionID session_id, char* buf, size_t size) = 0;
	virtual bool recv(TRequestID* req_id, bool* isTimeout, char* buf, size_t* size) = 0;

	TRequestID getNextRequestID() {
		return currentRequestID++;
	}
private:
	TRequestID currentRequestID = 0;
};

} // namespace transport
