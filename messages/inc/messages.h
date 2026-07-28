#pragma once

#include <array>
#include <asio.hpp>
#include <chrono>
#include <cstdint>

#include "constants.h"
#include "units.h"

namespace message {

struct SleepFor {
    std::chrono::milliseconds duration{};
};

struct CloseAllCameraWorkers {};

namespace led_matrix {

enum class color_t : int8_t { G = 'g', R = 'r', B = 'b' };

struct switch_to {
    uint8_t x{};
    uint8_t y{};
};

struct blank {};
struct next {};
}  // namespace led_matrix

namespace excitation {

struct laser {
    std::chrono::seconds time;
    uint8_t power;
    channel_t ch;
};

using namespace std::chrono_literals;
constexpr auto laser_off = laser{1s, 0, EGFP};

}  // namespace excitation

namespace motion {

struct move_to_z {
    units::Micron<> position;
};

}  // namespace motion
}  // namespace message
