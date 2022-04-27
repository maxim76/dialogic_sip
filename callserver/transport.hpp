#pragma once
#include <cstddef>

namespace transport {

typedef unsigned int TRequestID;

class Transport {
public:
	virtual ~Transport() {};
	// TODO: передавать буфер как std::string& чтоб обойтись одним параметром?
	virtual bool send( TRequestID req_id, char *buf, size_t size ) = 0;
	virtual bool recv( TRequestID *req_id, bool *isTimeout, char *buf, size_t *size ) = 0;
};

} // namespace transport
