#pragma once
#include "message_router.h"

namespace motion = message::motion;
namespace message_router {

template <class S>
void
MessageOverSerial<S>::sendCommand(motion::move_to_z z) {
    impl::executeCommand(serial, "z {:d}\n", z.position.value);
}
}  // namespace message_router
