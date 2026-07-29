#include <fmt/format.h>
#include <fmt/ranges.h>

#include <array>
#include <asio.hpp>

#include "message_router.h"
#include "mock_serial.h"

namespace message_router {

namespace impl {
template <class SerialInterface>
constexpr bool is_mock_serial_v = std::is_same_v<SerialInterface, hardware_drivers::MockSerial>;

template <class SerialInterface>
void
executeCommand(SerialInterface& serial, std::string_view message) {
    if constexpr (is_mock_serial_v<SerialInterface>) {
        fmt::print(FMT_STRING("Message -> {}: {:?}\n"), serial.port_name, message);
    } else {
        asio::write(serial, asio::buffer(message));
    }
}

template <class SerialInterface, typename FmtString, typename... Args>
void
executeCommand(SerialInterface& serial, FmtString fmt_string, Args&&... args) {
    constexpr auto size = 16;
    std::array<char, size> buffer;
    const auto [_, message_length] = fmt::format_to_n(
        buffer.begin(), size, std::forward<FmtString>(fmt_string), std::forward<Args>(args)...);

    if constexpr (is_mock_serial_v<SerialInterface>) {
        fmt::print(FMT_STRING("Message -> {}: {:?}\n"), serial.port_name,
                   std::string_view{buffer.data(), message_length});
    } else {
        asio::write(serial, asio::const_buffer{buffer.data(), message_length});
    }
    // asio::read_until(serial, asio::buffer(buffer), '\n');
}

template <class SerialInterface, class Cmd,
          std::enable_if_t<!std::is_convertible_v<Cmd, std::string_view>, int> = 0>
void
executeCommand(SerialInterface& serial, const Cmd cmd) {
    static_assert(std::is_trivially_copyable_v<Cmd>,
                  "Binary serial commands must be trivially copyable.");
    if constexpr (is_mock_serial_v<SerialInterface>) {
        fmt::print(
            FMT_STRING("Message -> {}: {{{:#02x}}}\n"), serial.port_name,
            fmt::join(std::string_view{reinterpret_cast<const char*>(&cmd), sizeof(cmd)}, ", "));
    } else {
        asio::write(serial, asio::buffer(static_cast<const void*>(&cmd), sizeof(cmd)));
    }
}

}  // namespace impl

}  // namespace message_router

#include "laser.hpp"
#include "led_matrix.hpp"
#include "motion.hpp"

template class message_router::MessageOverSerial<asio::serial_port>;

template class message_router::MessageOverSerial<hardware_drivers::MockSerial>;
