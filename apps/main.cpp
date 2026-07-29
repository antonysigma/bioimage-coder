#include <fmt/format.h>

#include <asio/io_service.hpp>
#include <asio/serial_port.hpp>

#include "file_write_worker.h"
#include "image_capture_worker.h"
#include "master_task.h"
#include "message_router.h"

using boost::fibers::barrier;
using boost::fibers::fiber;

#ifdef MOCK_SERIAL
#include "mock_serial.h"
hardware_drivers::MockSerial serial_port{"/dev/ttyACM0"};
#else
// Dependency injection of the serial port happens at the link-time of the
// binary.
asio::io_service io;
asio::serial_port serial_port{io};
#endif

// Dependency injection of the camera capture message router happens at the
// link-time of the binary.
using capture_queue_t = fiber_messages::capture::queue_t;
std::array image_capture_handlers{capture_queue_t{2}, capture_queue_t{2}, capture_queue_t{2},
                                  capture_queue_t{2}};

int
main() {
    // 96-eyes instrument's illumination/motion control is dispatched through
    // the Atmel ATMeta2560 AVR microcontroller.
    serial_port.open("/dev/ttyACM0");
    serial_port.set_option(asio::serial_port::baud_rate{115200U});

    fiber_messages::write::queue_t write_queue{4};

    std::array capture_tasks{
        fiber{imageCaptureWorker, 0, std::ref(image_capture_handlers[0]), std::ref(write_queue)},
        fiber{imageCaptureWorker, 1, std::ref(image_capture_handlers[1]), std::ref(write_queue)},
        fiber{imageCaptureWorker, 2, std::ref(image_capture_handlers[2]), std::ref(write_queue)},
        fiber{imageCaptureWorker, 3, std::ref(image_capture_handlers[3]), std::ref(write_queue)}};

    fiber executor_task{bioimageExecutorTask};

    std::thread write_task{fileWriteWorker, std::ref(write_queue)};

    // Not required if we never calls async_read or async_write.
    // io.run();
    executor_task.join();

    // Stop the asio event loop. Redundant call.
    // io.stop();

    for (auto& c : capture_tasks) {
        c.join();
    }

    write_task.join();

    return 0;
}
