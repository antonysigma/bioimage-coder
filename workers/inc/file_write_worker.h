#pragma once
#include <array>

#include "fiber-messages.h"

namespace workers {
struct FileWrite {
    inline static fiber_messages::write::queue_t write_queue{4};
    static void eventLoop();
};
}  // namespace workers