#pragma once

#include <string_view>

#include "messages.h"

/** Command serialization over serial port. */
namespace message_router {

/** Encode the instrument control messages to ASCII commands over the serial
 * port.
 *
 * The 96-eyes control instrument encodes the motion/illumination/thermal
 * commands as human-readable ASCII strings with the newline `\n` terminator.
 * This class utilizes C++ function overloading to map the messages from
 * C-structs to the corresponding encoder, so as to eliminate the runtime
 * overhead.
 *
 * For example, the C-struct led_matrix::switch_to has a pair of 8-bit unsigned
 * integer. The function sendCommand(switch_to) encodes it to `m XX YY\n`, and
 * then sends the ASCII string over the serial port.
 *
 * The serial port definition and implementation is injected at compile-time via
 * Dependency injection (DI).
 */
template <class SerialInterface>
class MessageOverSerial {
    SerialInterface& serial;

   public:
    inline MessageOverSerial(SerialInterface& s) : serial{s} {}

    MessageOverSerial(const MessageOverSerial&) = delete;
    MessageOverSerial(MessageOverSerial&&) = delete;
    MessageOverSerial(MessageOverSerial&) = delete;

    // White light llumination
    void sendCommand(message::led_matrix::switch_to);
    void sendCommand(message::led_matrix::color_t);
    void sendCommand(message::led_matrix::blank);
    void sendCommand(message::led_matrix::next);

    // Fluorescence excitation
    void sendCommand(message::excitation::laser);

    // Z-motion
    void sendCommand(message::motion::move_to_z);
};

}  // namespace message_router