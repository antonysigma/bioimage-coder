#pragma once
#include <cstdint>
#include <string_view>

namespace camera {
constexpr int32_t width = 2592;
constexpr int32_t height = 1944;
constexpr auto n_pixels = width * height;
}  // namespace camera

namespace well_plate {
constexpr int32_t n_wells = 96;
}

namespace frame_capture_card {
constexpr int32_t n_cameras_per_board = 24;
constexpr int32_t n_boards = 4;

static_assert(n_cameras_per_board * n_boards == ::well_plate::n_wells);
}  // namespace frame_capture_card

enum channel_t : uint8_t { EGFP = 1, TXRED = 0 };

constexpr std::string_view
toString(channel_t ch) {
    switch (ch) {
        case EGFP:
            return "EGFP";
        case TXRED:
            return "TXRED";
        default:
            return "NIL";
    }
}
