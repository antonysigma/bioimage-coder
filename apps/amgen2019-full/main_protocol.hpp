#pragma once

#include <chrono>
#include <cstdint>
#include <tuple>

#include "bioimage-coder/repeat-for.hpp"
#include "fiber-messages.h"
#include "frame-commands.h"
#include "messages.h"

using namespace std::chrono_literals;
using namespace message::led_matrix;
using namespace message::excitation;
using namespace message::motion;
using namespace message;
using fiber_messages::capture::dark_frame_t;
using fiber_messages::capture::fluorescence_frame_t;
using fiber_messages::capture::fpm_frame_t;
using next_illumination_angle = next;
using bioimage_coder::Range;
using bioimage_coder::repeat_for;
using message::CloseAllCameraWorkers;
using message::SleepFor;
using led_at = message::led_matrix::switch_to;
using fiber_messages::capture::camera::init_sequence_t;
using EG = fiber_messages::capture::camera::exposure_gain_t;

#define Steps std::tuple

namespace {

constexpr auto GREEN_LED = color_t::G;

constexpr auto
cameraInitProtocol() {
    using frame_capture_card::commands::i2c_cmd_t;

    return Steps{init_sequence_t{
        .commands =
            {// Mock sequence of CMOS sensor I2C commands to initialize all 96 cameras.
             i2c_cmd_t{0x0001, 0xff}, i2c_cmd_t{0x0002, 0xfe}, i2c_cmd_t{0x0003, 0xfd}},
        .count = 3}};
}

constexpr auto
fpmImagingProtocol(const uint8_t led_id) {
    return Steps{
        SleepFor{500ms},           // Wait for the z-stage to settle
        fpm_frame_t{led_id},       // Capture frames
        next_illumination_angle{}  // Move to the next LED
    };
}

constexpr auto
fluorescenceImagingProtocol(const units::Micron<> z) {
    return Steps{
        move_to_z{z},                         // Move to stage position
        laser{1s, 16, EGFP},                  // Turn on laser EGFP
        fluorescence_frame_t{z.value, EGFP},  // Capture fluorescence images
        laser{1s, 16, TXRED},                 // Turn on laser TXRED
        fluorescence_frame_t{z.value, TXRED}  // Capture fluorescence images
    };
}

constexpr auto
mainProtocol() {
    return Steps{
        cameraInitProtocol(),

        // Capture dark frames
        Steps{
            led_at{0, 0},                 //
            GREEN_LED,                    //
            blank{},                      //
            dark_frame_t{},               //
            led_at{0, 0},                 // Switch on LED
            move_to_z{0_um},              // Move to neutral position
            EG::setExposureGain<1>(30ms)  // Expose for 30 millisecond at 1x analog gain.
        },

        // Capture FPM images
        repeat_for(Range<'i', uint8_t>{0, 5}, fpmImagingProtocol),  //

        Steps{
            blank{},                        // Turn off LED.
            EG::setExposureGain<16>(200ms)  // Expose for 100 millisecond at 16x analog gain.
        },
        repeat_for(Range<'z', units::Micron<>>{-4_um, 4_um, 2_um}, fluorescenceImagingProtocol),  //

        // De-init all devices
        Steps{
            CloseAllCameraWorkers{},  // Closes all file writers
            laser_off,                //
            move_to_z{0_um}           // Move z-stage to neutral position
        }  //
    };
}

}  // namespace
