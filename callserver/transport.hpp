#pragma once
#include <cstddef>

namespace transport {

typedef unsigned int TRequestID;

class Transport {
public:
	virtual ~Transport() {};
	virtual bool send( TRequestID req_id, char *buf, size_t size ) = 0;
	virtual bool receive( TRequestID *req_id, bool *isTimeout, char *buf, size_t *size ) = 0;
};

} // namespace transport
