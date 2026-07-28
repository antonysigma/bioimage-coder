#include <fmt/chrono.h>
#include <fmt/format.h>

#include <array>
#include <chrono>
#include <tuple>

#include "units.h"

#ifndef BIOIMAGE_CODER_PROTOCOL_HEADER
#define BIOIMAGE_CODER_PROTOCOL_HEADER "main_protocol.hpp"
#endif

#include BIOIMAGE_CODER_PROTOCOL_HEADER

using bioimage_coder::is_repeat_for_v;
using bioimage_coder::Range;
using bioimage_coder::repeat_for;
using bioimage_coder::repeat_for_t;
using fiber_messages::capture::dark_frame_t;
using fiber_messages::capture::fluorescence_frame_t;
using fiber_messages::capture::fpm_frame_t;
using fiber_messages::capture::camera::exposure_gain_t;
using fiber_messages::capture::camera::init_sequence_t;
using message::CloseAllCameraWorkers;
using message::SleepFor;
using led_at = message::led_matrix::switch_to;

namespace {

template <typename T>
constexpr const T&
value_of(const T& value) {
    return value;
}

template <typename T>
constexpr const T&
value_of(const units::Micron<T>& value) {
    return value.value;
}

void
drawActivity(const dark_frame_t&) {
    fmt::print(":Capture dark frame;\n");
}

void
drawActivity(const fpm_frame_t&) {
    fmt::print(":Capture FPM images;\n");
}

void
drawActivity(const fluorescence_frame_t& frame) {
    fmt::print(FMT_STRING(":Capture fluorescence images for channel={:s};\n"), toString(frame.ch));
}

void
drawActivity(const exposure_gain_t& c) {
    fmt::print(FMT_STRING(":Set exposure={}; gain={:d}x;\n"), c.exposure(), c.gain());
}

void
drawActivity(const init_sequence_t&) {
    fmt::print(":Initialize CMOS sensors;\n");
}

void
drawActivity(const CloseAllCameraWorkers&) {
    fmt::print(":Closing cameras;\n");
}

void
drawActivity(const SleepFor& s) {
    fmt::print(FMT_STRING(":Wait for {:d} milliseconds;\n"), s.duration.count());
}

void
drawActivity(const move_to_z& m) {
    fmt::print(FMT_STRING(":Move to z = {:d} um;\n"), m.position.value);
}

void
drawActivity(const laser& l) {
    if (l.power <= 0) {
        fmt::print(":Laser off;\n");
        return;
    }
    fmt::print(FMT_STRING(":Laser {:s} at power {:d} for {:d} seconds;\n"), toString(l.ch), l.power,
               l.time.count());
}

void
drawActivity(const blank&) {
    fmt::print(":Turn off LED matrix;\n");
}

void
drawActivity(const led_at& l) {
    fmt::print(FMT_STRING(":Turn on LED at (x, y) = ({:d}, {:d});\n"), l.x, l.y);
}

void
drawActivity(const next&) {
    fmt::print(FMT_STRING(":Toggle the next LED;\n"));
}

void
drawActivity(const color_t& c) {
    fmt::print(FMT_STRING(":Set LED color = {:c};\n"), char(c));
}

template <char Symbol, typename Integer, class Callable>
void
drawActivity(const repeat_for_t<Symbol, Integer, Callable>& repeat_command) {
    fmt::print(FMT_STRING(":{:c} = {:d};\nrepeat\n"), decltype(repeat_command.range)::symbol,
               value_of(repeat_command.range.begin));

    std::apply([](auto&&... command) { (drawActivity(command), ...); },
               repeat_command.steps(repeat_command.range.begin));

    using namespace fmt::literals;
    fmt::print(R"(backward:{symbol:c} += {step:d};
repeat while ({symbol:c} < {end:d}?) is (yes)
->(no);
)",
               "symbol"_a = decltype(repeat_command.range)::symbol,
               "step"_a = value_of(repeat_command.range.step),
               "end"_a = value_of(repeat_command.range.end));
}

/** Black magic to dispatch commands in the tuple structure. */
template <typename Protocol, size_t index = 0>
constexpr void
drawPlantuml(Protocol&& p) {
    if constexpr (index < std::tuple_size_v<std::remove_reference_t<Protocol>>) {
        auto&& sub_protocol = std::get<index>(std::forward<Protocol>(p));
        using T = std::decay_t<decltype(sub_protocol)>;
        if constexpr (is_repeat_for_v<T>) {
            drawActivity(sub_protocol);
        } else {
            std::apply([](auto&&... command) { (drawActivity(command), ...); }, sub_protocol);
        }

        drawPlantuml<Protocol, index + 1>(std::forward<Protocol>(p));
    }
}

}  // namespace

int
main() {
    fmt::print("start\n");
    drawPlantuml(mainProtocol());
    fmt::print("stop\n");
    return 0;
}
