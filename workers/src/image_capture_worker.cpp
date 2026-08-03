#include "image_capture_worker.h"

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/std.h>

#include <bitset>
#include <string_view>
#include <type_traits>
#include <stdexcept>

//
#include <nonstd/span.hpp>

#include "file_write_worker.h"
#include "frame-capture-card.h"
#include "mock_usb.h"

using boost::this_fiber::sleep_for;
using boost::this_fiber::yield;
using namespace std::chrono_literals;
using boost::fibers::barrier;
using fiber_messages::capture::dark_frame_t;
using fiber_messages::capture::fluorescence_frame_t;
using fiber_messages::capture::fpm_frame_t;
using fiber_messages::capture::camera::exposure_gain_t;
using fiber_messages::capture::camera::init_sequence_t;
using frame_capture_card::n_cameras_per_board;
using frame_capture_card::commands::i2c_cmd_t;
using frame_capture_card::commands::write_led_id_t;
using hardware_drivers::MockUSB;
using message_router::FrameCaptureCard;
using std::chrono::steady_clock;
using frame_arrival_mask_t = std::bitset<n_cameras_per_board>;
using nonstd::span;

namespace {

constexpr auto all_frames_arrived = (uint32_t{1} << n_cameras_per_board) - 1;

/** Try to read frames from all 24 cameras. Giving up after 10 trials. */
template <class WriteMessage, class U>
frame_arrival_mask_t
captureFrom24Cameras(const uint8_t board_id, FrameCaptureCard<U>& capture_card,
                     const uint8_t target_led_id, fiber_messages::write::queue_t& write_queue,
                     uint16_t max_retry = 10) {
    // Allocate memory
    std::vector<uint8_t> image_buffer(camera::n_pixels);

    std::bitset<n_cameras_per_board> frame_arrival_mask{0U};
    for (size_t retry = 0; retry < max_retry * frame_capture_card::n_cameras_per_board; retry++) {
        const auto ret = capture_card.captureSingleFrame(image_buffer);

        if (ret.led_id != target_led_id) continue;

        // Mark the i-th camera as captured.
        frame_arrival_mask.set(ret.cam_id - 1);

        // Transmit the frame to the write queue
        std::vector<uint8_t> captured_image(camera::n_pixels);
        std::swap(captured_image, image_buffer);

        write_queue.push(fiber_messages::write::command_t{
            std::in_place_type<WriteMessage>,
            board_id,
            ret,
            std::move(captured_image),
        });

        if (frame_arrival_mask == all_frames_arrived) {
            break;
        }
    }

    return frame_arrival_mask;
}

template <class U, uint16_t max_retry = 10>
void
execute(const uint8_t board_id, FrameCaptureCard<U>& capture_card, const dark_frame_t& cmd,
        fiber_messages::write::queue_t& write_queue) {
    fmt::print(FMT_STRING("[{:d}] Capture darkframe...\n"), board_id);
    static uint8_t frame_id{0};
    assert(capture_card.sendCommand(write_led_id_t{++frame_id}));

    // Stream frames from 24 cameras to the write queue.
    const auto frame_arrival_mask = captureFrom24Cameras<fiber_messages::write::dark_frame_t>(
        board_id, capture_card, frame_id, write_queue);
    if (frame_arrival_mask != all_frames_arrived) {
        fmt::print(FMT_STRING("[{:d}] Warning: not all frames arrived.\n"), board_id);
    }

    // Send completion signal to main loop
    assert(cmd.completion != nullptr);
    cmd.completion->push(true);
}

template <class U>
void
execute(const uint8_t board_id, FrameCaptureCard<U>& capture_card,
        const fpm_frame_t& capture_command, fiber_messages::write::queue_t& write_queue) {
    fmt::print(FMT_STRING("[{:d}] Capture FPM frame {:d}...\n"), board_id, capture_command.led_id);
    assert(capture_card.sendCommand(write_led_id_t{capture_command.led_id}));

    // Transfer images from camera board
    const auto frame_arrival_mask = captureFrom24Cameras<fiber_messages::write::fpm_frame_t>(
        board_id, capture_card, capture_command.led_id, write_queue);
    if (frame_arrival_mask != all_frames_arrived) {
        fmt::print(FMT_STRING("[{:d}] Warning: not all frames arrived.\n"), board_id);
    }

    // Send completion signal to the main loop
    assert(capture_command.completion != nullptr);
    capture_command.completion->push(true);
}

template <class U, uint8_t n_frames = 8>
void
execute(const uint8_t board_id, FrameCaptureCard<U>& capture_card,
        const fluorescence_frame_t& capture_command, fiber_messages::write::queue_t& write_queue) {
    using frame_capture_card::n_cameras_per_board;

    const uint8_t frame_id =
        (capture_command.zpos * 2 + static_cast<uint8_t>(capture_command.ch)) & 0xff;
    assert(capture_card.sendCommand(write_led_id_t{frame_id}));

    // Allocate memory
    std::array<std::vector<uint16_t>, n_cameras_per_board> accumulated{};
    std::vector<uint8_t> raw_pixels(camera::n_pixels);

    for (auto& frame : accumulated) {
        frame.resize(camera::n_pixels);
    }

    // Time integration count
    std::array<uint8_t, n_cameras_per_board> accumulated_frame_count{};

    const size_t max_retry = 10 * n_cameras_per_board * n_frames;
    // Accumulate intensity
    for (size_t retry = 0; retry < max_retry; retry++) {
        const auto [cam_id, led_id] = capture_card.captureSingleFrame(raw_pixels);

        // Skip frame if it is captured before the laser trigger.
        if (led_id != frame_id) continue;

        // Skip frame if sufficient number of frames is time-integrated for the camera.
        auto& frame_count = accumulated_frame_count.at(cam_id - 1);
        if (frame_count >= n_frames) continue;

        fmt::print(
            "[{:d}] Flurescence time integration {:d}/{:d} at [z={:d}, "
            "ch={:s}, cam={:d}]...\n",
            board_id, frame_count, n_frames, capture_command.zpos, toString(capture_command.ch),
            cam_id);

#ifndef __OPTIMIZE__
#warning Digital time integration of image frames takes too long. Try compiler arguments -march=native -O3
#endif

        // Now, perform digital time integration
        auto& target_frame = accumulated.at(cam_id - 1);
        std::transform(raw_pixels.begin(), raw_pixels.end(), target_frame.begin(),
                       target_frame.begin(),
                       [](const auto x, const auto y) -> uint16_t { return x + y; });
        yield();

        // If time intergration is complete, transmit the frame of the corresponding camera once and
        // only once.
        if (++frame_count == n_frames) {
            write_queue.push(fiber_messages::write::command_t{
                std::in_place_type<fiber_messages::write::fluorescence_frame_t>,
                board_id,
                cam_id,
                capture_command.zpos,
                capture_command.ch,
                std::move(target_frame),
            });
        }

        // Break loop early if all cameras successfully performs time integration.
        if (std::all_of(accumulated_frame_count.begin(), accumulated_frame_count.end(),
                        [](const auto& c) -> bool { return c >= n_frames; })) {
            break;
        }
    }

    // Send the completion signal to the main loop
    assert(capture_command.completion != nullptr);
    capture_command.completion->push(true);
}

template <class U>
void
execute(const uint8_t board_id, FrameCaptureCard<U>& capture_card,
        const init_sequence_t& capture_command) {
    fmt::print(FMT_STRING("[{:d}] Camera init sequence...\n"), board_id);

    for (const auto& i2c_cmd : span{capture_command.commands}.subspan(0, capture_command.count)) {
        const bool is_transmitted = capture_card.sendCommand(i2c_cmd);
        if (!is_transmitted) {
            fmt::print(FMT_STRING("[{:d}] i2c command failed: {:#x},{:#x}={:#x}"), board_id,
                       i2c_cmd.dev_addr, i2c_cmd.addr, i2c_cmd.value);
        }
        yield();
    }
}

template <class U>
void
execute(const uint8_t board_id, FrameCaptureCard<U>& capture_card,
        const exposure_gain_t& capture_command) {
    fmt::print(FMT_STRING("[{:d}] Set exposure = {}, gain={:d}x\n"), board_id,
               capture_command.exposure(), capture_command.gain());

    static_assert(sizeof(exposure_gain_t) % sizeof(i2c_cmd_t) == 0);
    constexpr int32_t n_commands = sizeof(exposure_gain_t) / sizeof(i2c_cmd_t);
    for (const auto& i2c_cmd :
         span{reinterpret_cast<const i2c_cmd_t*>(&capture_command), n_commands}) {
        const bool is_transmitted = capture_card.sendCommand(i2c_cmd);
        if (!is_transmitted) {
            fmt::print(FMT_STRING("[{:d}] i2c command failed: {:#x},{:#x}={:#x}"), board_id,
                       i2c_cmd.dev_addr, i2c_cmd.addr, i2c_cmd.value);
        }
        yield();
    }
}
}  // namespace

namespace workers {
template <uint8_t usb_id, class FileWriterWorker>
void
ImageCapture<usb_id, FileWriterWorker>::eventLoop() {
    // Initialize camera board
    message_router::FrameCaptureCard<MockUSB> capture_card{usb_id};
    const auto board_id = capture_card.readBoardID();

    for (auto&& cmd : capture_queue) {
        using namespace std::string_view_literals;

        // Write to file
        std::visit(
            [&](auto&& capture_command) {
                using T = std::decay_t<decltype(capture_command)>;
                if constexpr (std::is_same_v<T, dark_frame_t> || std::is_same_v<T, fpm_frame_t> ||
                              std::is_same_v<T, fluorescence_frame_t>) {
                    execute(board_id, capture_card, capture_command, FileWriterWorker::write_queue);
                } else {
                    execute(board_id, capture_card, capture_command);
                }
            },
            cmd);
    }

    if (board_id == 0) {
        std::puts("[0] No more instrument control commands. Closing the file worker...\n");
        FileWriterWorker::write_queue.close();
    }

    // Need to capture the board_id value from the stack before it vanishes from the stack.
    [=]() { fmt::print(FMT_STRING("[{:d}] Closing image capture worker\n"), board_id); }();
}

template class ImageCapture<0, FileWrite>;
template class ImageCapture<1, FileWrite>;
template class ImageCapture<2, FileWrite>;
template class ImageCapture<3, FileWrite>;
}  // namespace workers