#pragma once
#include <memory>
#include "transport.hpp"

namespace transport {

std::unique_ptr<Transport> createTransportZMQ();

} // namespace transport
