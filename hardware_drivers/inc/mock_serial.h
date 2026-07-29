#pragma once
#include <string_view>

namespace hardware_drivers {
struct MockSerial {
    std::string_view port_name{};

    template <class IO>
    constexpr MockSerial(IO&) {}

    MockSerial(const MockSerial&) = delete;
    MockSerial(MockSerial&&) = delete;
    MockSerial(MockSerial&) = delete;

    template <class T>
    constexpr void set_option(const T&) const noexcept {}

    void open(std::string_view name) noexcept { port_name = name; }

    constexpr void close() const noexcept {}
};

}  // namespace hardware_drivers