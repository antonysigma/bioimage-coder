#pragma once

#include <chrono>
#include <cstdint>
#include <tuple>

#include "bioimage-coder/repeat-for.hpp"
#include "fiber-messages.h"
#include "messages.h"

using namespace std::chrono_literals;
using namespace message::led_matrix;
using namespace message::excitation;
using namespace message::motion;
using namespace message;
using bioimage_coder::Range;
using bioimage_coder::repeat_for;
using fiber_messages::capture::dark_frame_t;
using fiber_messages::capture::fluorescence_frame_t;
using message::CloseAllCameraWorkers;
using message::SleepFor;
using led_at = message::led_matrix::switch_to;

#define Steps std::tuple

namespace {

constexpr auto z_range = 4_um;

constexpr auto GREEN_LED = color_t::G;
constexpr auto
fluorescenceImagingProtocol(const units::Micron<> z) {
    return Steps{
        move_to_z{z},                          // Move to stage position
        SleepFor{100ms},                       // Wait for the z-stage to settle
        laser{1s, 16, EGFP},                   // Turn on laser EGFP
        fluorescence_frame_t{z.value, TXRED},  // Capture fluorescence images
        laser{1s, 16, EGFP},                   // Turn on laser TXRED
        fluorescence_frame_t{z.value, TXRED}   // Capture fluorescence images
    };
}

constexpr auto
mainProtocol() {
    return Steps{
        //

        // Capture dark frames
        Steps{
            move_to_z{0_um},  // Move to neutral position
            led_at{0, 0},     //
            GREEN_LED,        //
            blank{},          //
            dark_frame_t{},   //
        },

        Steps{
            move_to_z{-z_range},  // Move to the start of the z range
            SleepFor{500ms}       // Wait for the z-stage to settle
        },

        repeat_for(Range<'z', units::Micron<>>{-z_range, z_range, 2_um},
                   fluorescenceImagingProtocol),  //

        // De-init all devices
        Steps{
            CloseAllCameraWorkers{},  // Closes all file writers
            laser_off,                //
            move_to_z{0_um}           // Move z-stage to neutral position
        }  //
    };
}

}  // namespace
