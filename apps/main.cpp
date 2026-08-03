#include <fmt/format.h>

#include <asio/io_service.hpp>
#include <asio/serial_port.hpp>

#include "file_write_worker.h"
#include "image_capture_worker.h"
#include "master_task.hpp"
#include "message_router.h"
#include "mock_serial.h"

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

// Dependency injection of the camera capture message router happens at compile-time.
using workers::FileWrite;
using workers::ImageCapture;
using MessageOverSerial = message_router::MessageOverSerial<hardware_drivers::MockSerial>;

int
main() {
    // 96-eyes instrument's illumination/motion control is dispatched through
    // the Atmel ATMeta2560 AVR microcontroller.
    serial_port.open("/dev/ttyACM0");
    serial_port.set_option(asio::serial_port::baud_rate{115200U});

    std::array capture_tasks{
        fiber{ImageCapture<0, FileWrite>::eventLoop},
        fiber{ImageCapture<1, FileWrite>::eventLoop},
        fiber{ImageCapture<2, FileWrite>::eventLoop},
        fiber{ImageCapture<3, FileWrite>::eventLoop},
    };

    fiber executor_task{workers::bioimageExecutorTask<  //
        MessageOverSerial,                              //
        ImageCapture<0, FileWrite>,                     //
        ImageCapture<1, FileWrite>,                     //
        ImageCapture<2, FileWrite>,                     //
        ImageCapture<3, FileWrite>                      //
        >};

    std::thread write_task{FileWrite::eventLoop};

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
