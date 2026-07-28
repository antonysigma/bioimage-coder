#pragma once
#include <fmt/format.h>

#include <cstdint>

namespace units {

template <typename T = int16_t>
struct Micron {
    T value{};
};

/** Syntactic function for micrometer unit. */
constexpr Micron<int16_t>
operator""_um(unsigned long long v) {
    return Micron<int16_t>{static_cast<int16_t>(v)};
}

template <typename T>
constexpr bool
operator<(Micron<T> lhs, Micron<T> rhs) {
    return lhs.value < rhs.value;
}

template <typename T>
constexpr bool
operator==(Micron<T> lhs, Micron<T> rhs) {
    return lhs.value == rhs.value;
}

template <typename T>
constexpr Micron<T>
operator+(Micron<T> lhs, Micron<T> rhs) {
    return Micron<T>{static_cast<T>(lhs.value + rhs.value)};
}

template <typename T>
constexpr Micron<T>
operator-(Micron<T> lhs, Micron<T> rhs) {
    return Micron<T>{static_cast<T>(lhs.value - rhs.value)};
}

template <typename T>
constexpr Micron<T>
operator-(Micron<T> value) {
    return Micron<T>{static_cast<T>(-value.value)};
}

template <typename T>
constexpr Micron<T>&
operator+=(Micron<T>& lhs, Micron<T> rhs) {
    lhs.value = static_cast<T>(lhs.value + rhs.value);
    return lhs;
}

template <typename T>
constexpr Micron<T>&
operator-=(Micron<T>& lhs, Micron<T> rhs) {
    lhs.value = static_cast<T>(lhs.value - rhs.value);
    return lhs;
}

}  // namespace units

using units::operator""_um;

namespace fmt {

template <typename T>
struct formatter<units::Micron<T>> {
    fmt::formatter<T> value_formatter{};

    constexpr auto parse(format_parse_context& ctx) { return value_formatter.parse(ctx); }

    template <typename FormatContext>
    auto format(const units::Micron<T>& m, FormatContext& ctx) const {
        auto out = value_formatter.format(m.value, ctx);
        return fmt::format_to(out, FMT_STRING(" um"));
    }
};

}  // namespace fmt
