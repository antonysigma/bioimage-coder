#include "file_write_worker.h"

#include <fmt/format.h>

#include <boost/fiber/all.hpp>

using fiber_messages::write::command_t;
using fiber_messages::write::dark_frame_t;
using fiber_messages::write::fluorescence_frame_t;
using fiber_messages::write::fpm_frame_t;

void
fileWriteWorker(fiber_messages::write::queue_t& write_queue) {
    using namespace std::string_view_literals;
    constexpr auto success = boost::fibers::channel_op_status::success;

    command_t cmd;

    while (write_queue.pop(cmd) == success) {
        std::visit(
            [](auto&& frame) {
                using T = std::decay_t<decltype(frame)>;
                static_assert(!std::is_copy_constructible_v<T>, "Buffer should support zero-copy.");

                if constexpr (std::is_same_v<T, dark_frame_t>) {
                    fmt::print(FMT_STRING("[{:d}] Writing darkframe from camera {:d}...\n"),
                               frame.board_id, frame.cam_id);
                } else if constexpr (std::is_same_v<T, fpm_frame_t>) {
                    fmt::print(FMT_STRING("[{:d}] Writing FPM frame {:d} from camera {:d}...\n"),
                               frame.board_id, frame.led_id, frame.cam_id);
                } else if constexpr (std::is_same_v<T, fluorescence_frame_t>) {
                    fmt::print(FMT_STRING("[{:d}] Writing fluorescence frame at [z={:d}, ch={:s}] "
                                          "from camera {:d}...\n"),
                               frame.board_id, frame.zpos, toString(frame.ch), frame.cam_id);
                } else {
                    static_assert(sizeof(T) == 0, "File write command not recognized");
                }
            },
            std::move(cmd));
    }

    std::puts("[ ] Closing file worker\n");
}