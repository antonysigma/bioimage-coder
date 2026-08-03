#pragma once
#include <array>
#include <boost/fiber/barrier.hpp>
#include <cstdint>

#include "fiber-messages.h"

namespace workers {
template <uint8_t board_id, class FileWriter>
struct ImageCapture {
    static inline fiber_messages::capture::queue_t capture_queue{2};
    static void eventLoop();
};
}  // namespace workers