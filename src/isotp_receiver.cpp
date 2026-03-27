#include <cstdint>
#include <cstring>
#include <iostream>

using u8  = uint8_t;
using u16 = uint16_t;

// Fixed limits used by the ISO-TP receiver simulation.
// Using named constants improves readability and reduces magic numbers.
constexpr u16 MAX_ISOTP_PAYLOAD            = 4096U;
constexpr u8  SINGLE_FRAME_MAX_DATA_BYTES  = 7U;
constexpr u8  FIRST_FRAME_DATA_BYTES       = 6U;
constexpr u8  CONSECUTIVE_FRAME_DATA_BYTES = 7U;

// ISO-TP frame types encoded in the high nibble of the PCI byte.
enum class FrameType : u8 {
    SingleFrame      = 0x0U,
    FirstFrame       = 0x1U,
    ConsecutiveFrame = 0x2U
};

// Return status used to make frame processing more explicit.
enum class ProcessStatus : u8 {
    Ok,
    NullPointer,
    InvalidSingleFrameLength,
    InvalidFirstFrameLength,
    OversizedMessage,
    UnexpectedConsecutiveFrame,
    UnsupportedFrameType
};

// Receiver-side state used to reassemble a multi-frame ISO-TP message.
struct IsoTpMessageState {
    u8  buffer[MAX_ISOTP_PAYLOAD];
    u16 bytes_received;
    u16 total_length;
    bool message_complete;
};

static IsoTpMessageState rx_state = {};

// Reset the receiver state before starting a new message.
static void reset_state()
{
    rx_state.bytes_received   = 0U;
    rx_state.total_length     = 0U;
    rx_state.message_complete = false;
}

// Simulate transmission of an ISO-TP Flow Control frame.
// In a real ECU, this frame would be sent over CAN to allow the sender to continue.
static void send_flow_control()
{
    // Byte 0: 0x30 -> Flow Control + Clear To Send (CTS)
    // Byte 1: Block Size = 0 -> sender may continue without interruption
    // Byte 2: STmin = 0x0A -> 10 ms minimum separation time
    const u8 fc_frame[8] = {0x30U, 0x00U, 0x0AU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U};

    (void)fc_frame; // Frame is only simulated in this version.

    std::cout << "   [TX] Sending FLOW CONTROL (CTS)\n";
    std::cout << "   [TX] Block Size: 0 (Send all remaining frames)\n";
    std::cout << "   [TX] STmin: 10 ms\n";
}

// Process one received CAN frame containing ISO-TP transport data.
static ProcessStatus process_frame(const u8* const frame)
{
    if (frame == nullptr) {
        std::cout << "ERROR: Null frame pointer.\n";
        return ProcessStatus::NullPointer;
    }

    // PCI = Protocol Control Information byte.
    // High nibble identifies the ISO-TP frame type.
    const u8 pci         = frame[0];
    const u8 frame_nib   = static_cast<u8>(pci >> 4U);
    const FrameType type = static_cast<FrameType>(frame_nib);

    switch (type) {
        case FrameType::SingleFrame: {
            std::cout << "[Detected] SINGLE FRAME\n";

            // Low nibble contains payload length for Single Frame.
            const u8 payload_length = static_cast<u8>(pci & 0x0FU);

            if (payload_length > SINGLE_FRAME_MAX_DATA_BYTES) {
                std::cout << "ERROR: Invalid Single Frame length.\n";
                return ProcessStatus::InvalidSingleFrameLength;
            }

            reset_state();
            rx_state.total_length = payload_length;

            // Copy payload directly from bytes 1..n.
            (void)std::memcpy(rx_state.buffer, &frame[1], payload_length);

            rx_state.bytes_received   = payload_length;
            rx_state.message_complete = true;

            std::cout << "-> Payload length: " << static_cast<int>(payload_length) << " bytes\n";
            std::cout << "-> Status: COMPLETE\n";

            return ProcessStatus::Ok;
        }

        case FrameType::FirstFrame: {
            std::cout << "\n[Detected] FIRST FRAME\n";

            // First Frame length uses 12 bits:
            // lower nibble of byte 0 + full byte 1.
            const u16 requested_length =
                static_cast<u16>((static_cast<u16>(frame[0] & 0x0FU) << 8U) |
                                 static_cast<u16>(frame[1]));

            // Reject messages larger than the fixed receiver buffer.
            if (requested_length > MAX_ISOTP_PAYLOAD) {
                std::cout << "ERROR: Message too large (" << requested_length << " bytes).\n";
                std::cout << "-> Buffer limit is " << MAX_ISOTP_PAYLOAD << " bytes. Request rejected.\n";
                return ProcessStatus::OversizedMessage;
            }

            // A valid First Frame must describe more data than fits in a Single Frame.
            if (requested_length < (FIRST_FRAME_DATA_BYTES + 1U)) {
                std::cout << "ERROR: Invalid First Frame length.\n";
                return ProcessStatus::InvalidFirstFrameLength;
            }

            reset_state();
            rx_state.total_length = requested_length;

            // First Frame carries the first 6 payload bytes after the 2-byte PCI/length header.
            (void)std::memcpy(rx_state.buffer, &frame[2], FIRST_FRAME_DATA_BYTES);
            rx_state.bytes_received = FIRST_FRAME_DATA_BYTES;

            std::cout << "-> Total message size: " << rx_state.total_length << " bytes\n";
            std::cout << "-> Captured initial " << static_cast<int>(FIRST_FRAME_DATA_BYTES) << " bytes\n";
            std::cout << "-> Status: WAITING ("
                      << static_cast<u16>(rx_state.total_length - rx_state.bytes_received)
                      << " bytes remaining)\n";

            send_flow_control();
            return ProcessStatus::Ok;
        }

        case FrameType::ConsecutiveFrame: {
            std::cout << "\n[Detected] CONSECUTIVE FRAME\n";

            // A Consecutive Frame is only valid when a multi-frame reception is active.
            if ((rx_state.total_length == 0U) || rx_state.message_complete) {
                std::cout << "ERROR: Unexpected Consecutive Frame. No active multi-frame reception.\n";
                return ProcessStatus::UnexpectedConsecutiveFrame;
            }

            // Low nibble contains the sequence number.
            const u8 sequence_number = static_cast<u8>(pci & 0x0FU);

            // Determine how many bytes are still missing.
            const u16 remaining_bytes =
                static_cast<u16>(rx_state.total_length - rx_state.bytes_received);

            // A Consecutive Frame can carry up to 7 data bytes.
            const u8 chunk_size =
                (remaining_bytes > CONSECUTIVE_FRAME_DATA_BYTES)
                    ? CONSECUTIVE_FRAME_DATA_BYTES
                    : static_cast<u8>(remaining_bytes);

            std::cout << "-> Sequence number: " << static_cast<int>(sequence_number) << "\n";

            // Append the received data to the reconstruction buffer.
            (void)std::memcpy(&rx_state.buffer[rx_state.bytes_received], &frame[1], chunk_size);

            rx_state.bytes_received =
                static_cast<u16>(rx_state.bytes_received + static_cast<u16>(chunk_size));

            if (rx_state.bytes_received >= rx_state.total_length) {
                rx_state.message_complete = true;
                std::cout << "-> Status: COMPLETE MESSAGE ASSEMBLED\n";
            } else {
                std::cout << "-> Status: WAITING ("
                          << static_cast<u16>(rx_state.total_length - rx_state.bytes_received)
                          << " bytes remaining)\n";
            }

            return ProcessStatus::Ok;
        }

        default:
            std::cout << "ERROR: Unsupported or unknown ISO-TP frame type.\n";
            return ProcessStatus::UnsupportedFrameType;
    }
}

int main()
{
    std::cout << "--- ISO-TP RECEIVER SIMULATION START ---\n\n";

    // Demo frame 1:
    // First Frame announcing a 13-byte message and carrying the first 6 bytes.
    const u8 frame1[8] = {0x10U, 0x0DU, 0xAAU, 0xBBU, 0xCCU, 0xDDU, 0xEEU, 0xFFU};
    (void)process_frame(frame1);

    std::cout << "\n----------------------------------------\n";

    // Demo frame 2:
    // Consecutive Frame carrying the remaining 7 bytes of the message.
    const u8 frame2[8] = {0x21U, 0x11U, 0x22U, 0x33U, 0x44U, 0x55U, 0x66U, 0x77U};
    (void)process_frame(frame2);

    std::cout << "\n--- SIMULATION END ---\n";
    return 0;
}
