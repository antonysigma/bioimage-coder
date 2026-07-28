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

constexpr auto GREEN_LED = color_t::G;
constexpr auto
brightfieldImagingProtocol(const units::Micron<> z) {
    return Steps{
        move_to_z{z},     // Move to stage position
        SleepFor{500ms},  // Wait for the z-stage to settle
        fluorescence_frame_t{static_cast<uint8_t>(z.value + 128)},  // Capture frames
    };
}

constexpr auto
mainProtocol() {
    return Steps{
        //

        // Capture dark frames
        Steps{
            led_at{0, 0},    //
            GREEN_LED,       //
            blank{},         //
            dark_frame_t{},  //
        },

        // Switch on the center LED for well A1. Owing to the layout of the LED
        // matrix, not all wells on the 96-well plate can be illuminated with
        // on-axis LEDs.
        Steps{
            led_at{0, 0},     // Switch on LED
            move_to_z{0_um},  // Move to neutral position
        },

        repeat_for(Range<'z', units::Micron<>>{-4_um, 4_um, 2_um}, brightfieldImagingProtocol),  //

        // De-init all devices
        Steps{
            CloseAllCameraWorkers{},  // Closes all file writers
            laser_off,                //
            move_to_z{0_um}           // Move z-stage to neutral position
        }  //
    };
}

}  // namespace
