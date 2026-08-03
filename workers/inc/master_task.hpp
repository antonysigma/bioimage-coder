#pragma once
#include <fmt/format.h>

#include "bioimage-coder/executor.hpp"
#include "fiber-messages.h"
#include "main_protocol.hpp"

namespace workers {
template <class MessageOverSerial, class... ImageCaptureWorkers>
void
bioimageExecutorTask() {
    try {
        bioimage_coder::Executor<MessageOverSerial, ImageCaptureWorkers...>::execute(
            mainProtocol());

        // Close serial port.
        serial_port.close();

    } catch (const asio::system_error& e) {
        fmt::print(stderr, FMT_STRING("ASIO error: {:s}\n"), e.what());
        (ImageCaptureWorkers::capture_queue.close(), ...);
    }
}
}  // namespace workers