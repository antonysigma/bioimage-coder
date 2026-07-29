#pragma once
#include <array>
#include <asio/io_service.hpp>
#include <asio/serial_port.hpp>
#include <boost/fiber/all.hpp>

#include "bioimage-coder/repeat-for.hpp"

#ifndef USING_FIBER
#include <thread>
#endif

#include "constants.h"
#include "fiber-messages.h"
#include "message_router.h"
#include "messages.h"

#ifdef MOCK_SERIAL
#include "mock_serial.h"
using serial_port_t = hardware_drivers::MockSerial;
#else
using serial_port_t = asio::serial_port;
#endif

/** Dependency injection (DI) of the serial port at link-time.
 *
 * @todo to be refactored into compile-time DI via C++ template metaprogramming.
 */
extern serial_port_t serial_port;

/** Dependency injection (DI) of the capture command queue at link-time.
 *
 * @todo to be refactored into compile-time DI via C++ template metaprogramming.
 */
extern std::array<fiber_messages::capture::queue_t, frame_capture_card::n_boards>
    image_capture_handlers;

namespace bioimage_coder {

template <typename, typename>
constexpr bool is_in_variant = false;

template <typename T, typename... Ts>
constexpr bool is_in_variant<T, std::variant<Ts...>> = (std::is_same_v<T, Ts> || ...);

template <typename T, typename = void>
struct has_completion_channel : std::false_type {};

template <typename T>
struct has_completion_channel<T, std::void_t<decltype(T{}.completion)>> : std::true_type {};

template <char Symbol, typename Integer, class Callable>
constexpr void dispatch(const repeat_for_t<Symbol, Integer, Callable>& command);

/** Dispatch the steps in Bioimage Coder DSL to the corresponding message
 * handlers.
 *
 * 96-eyes control instrument has two main ports: UART-over-USB2.0 port, and
 * the FPGA-over-USB3.0 ports.
 *
 * There, the job of the dispatcher is to determine (at compile time) if the
 * incoming command is related to image capture. if yes, the command is routed
 * to the FPGA-over-USB3.0 port. The command queue is currently
 * implemented with boost::buffered_channel to take advantage of the RTOS-style
 * programming for concurrent instrument control.
 *
 * For illumination/motion/thermal control commands, the dispatcher (again at
 * compile time) routes the command to the UART-over-USB2.0 port.
 *
 * All these commands are resolved entirely at compile time, ensuring zero
 * runtime dispatch overhead. Hence, the risk of photobleaching and
 * exposure/position synchronization is eliminated at the root.
 *
 * The dependency injection (DI) of the hardware ports (UART and the custom USB3.0
 * drivers) are currently achieved at link-time with the C++ toolchain.
 *
 * @todo Refactor the DI from link-time to compile-time.
 */
template <typename T>
constexpr void
dispatch(const T& command) {
    using namespace message;

    using Type = std::remove_reference_t<T>;

    if constexpr (std::is_same_v<Type, CloseAllCameraWorkers>) {
        for (auto& capture_port : image_capture_handlers) {
            capture_port.close();
        }
    } else if constexpr (is_in_variant<Type, fiber_messages::capture::command_t>) {
        if constexpr (has_completion_channel<Type>::value) {
            using frame_capture_card::n_boards;
            fiber_messages::capture::completions_signal_t completion{n_boards};

            Type new_capture_command{command};
            new_capture_command.completion = &completion;
            for (auto& capture_port : image_capture_handlers) {
                capture_port.push(new_capture_command);
            }

            // Suspend master loop until all capture workers report capture complete.
            for (uint8_t i = 0; i < n_boards; i++) {
                bool ack;
                completion.pop(ack);
            }
        } else {
            for (auto& capture_port : image_capture_handlers) {
                capture_port.push(command);
            }
        }
    } else if constexpr (std::is_same_v<Type, SleepFor>) {
#ifdef USING_FIBER
        boost::this_fiber::sleep_for(command.duration);
#else
        std::this_thread::sleep_for(command.duration);
#endif
    } else {
        message_router::MessageOverSerial{serial_port}.sendCommand(command);
    }
}

template <char Symbol, typename Integer, class Callable>
constexpr void
dispatch(const repeat_for_t<Symbol, Integer, Callable>& command) {
    for (auto i = command.range.begin; i < command.range.end; i += command.range.step) {
        std::apply([](auto&&... nested_command) { (dispatch(nested_command), ...); },
                   command.steps(i));
    }
}

/** Black magic to dispatch commands in the tuple structure. */
template <typename Protocol, size_t index = 0>
constexpr void
execute(Protocol&& p) {
    if constexpr (index < std::tuple_size_v<std::remove_reference_t<Protocol>>) {
        // Retrieve the current step
        auto&& sub_protocol = std::get<index>(std::forward<Protocol>(p));

        // If the step contains a loop, repeat the steps in the loop N times.
        using T = std::decay_t<decltype(sub_protocol)>;
        if constexpr (is_repeat_for_v<T>) {
            for (auto i = sub_protocol.range.begin; i < sub_protocol.range.end;
                 i += sub_protocol.range.step) {
                std::apply([](auto&&... command) { (dispatch(command), ...); },
                           sub_protocol.steps(i));
            }
        } else {
        // Otherwise, dispatch the commands once.
            std::apply([](auto&&... command) { (dispatch(command), ...); }, sub_protocol);
        }

        // Compile the next step.
        execute<Protocol, index + 1>(std::forward<Protocol>(p));
    }
}
}  // namespace bioimage_coder
