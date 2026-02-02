#include <iostream>
#include <cstdint>
#include <cstring>
#include <iomanip>

using u8 = uint8_t;
using u16 = uint16_t;

struct data {
    u8 buffer[4096];
    u16 bytes_received;
    u16 total_length; 
    bool message_complete;
};

static data mydata;

void process_frame(const u8 *ptr) {
    u8 byte0 = *ptr;
    u8 type = byte0 >> 4;

    switch (type) {
        // CASE 0: Single Frame
        case 0x00: { 
            std::cout << "[detected] SINGLE FRAME (Length < 8)\n";
            u8 leng = byte0 & 0x0F;
            
            mydata.total_length = leng;
            std::memcpy(mydata.buffer, &ptr[1], leng);
            
            mydata.bytes_received = leng;
            mydata.message_complete = true;
            
            std::cout << "-> Captured " << (int)mydata.bytes_received << " bytes.\n";
            std::cout << "-> Status: COMPLETE.\n";
            break;
        } 

        // CASE 1: First Frame
        case 0x01: { 
            std::cout << "\n[Detected] FIRST FRAME (Type 1)\n";
            
            // Reset for new message
            mydata.bytes_received = 0;
            mydata.message_complete = false;

            // Combine two bytes for length
            u16 leng = ((ptr[0] & 0x0F) << 8) | ptr[1];
            mydata.total_length = leng;
            
            std::cout << "-> Total Message Size: " << (int)mydata.total_length << " bytes.\n";

            // Copy first 6 bytes
            std::memcpy(mydata.buffer, &ptr[2], 6);
            mydata.bytes_received = 6;
            
            std::cout << "-> Status: WAITING (Need " << (mydata.total_length - 6) << " more bytes).\n";
            break;
        }

        // CASE 2: Consecutive Frame
        case 0x02: { 
            std::cout << "\n[Detected] CONSECUTIVE FRAME (Type 2)\n";
            
            u16 remaining = mydata.total_length - mydata.bytes_received;
            u8 chunk;
            
            if (remaining > 7) {
                chunk = 7;
            } else {
                chunk = remaining;
            }

            // Append to the end of the buffer
            std::memcpy(&mydata.buffer[mydata.bytes_received], &ptr[1], chunk);
            
            mydata.bytes_received += chunk;

            if (mydata.bytes_received >= mydata.total_length) {
                mydata.message_complete = true;
                std::cout << "-> Status: COMPLETE MESSAGE ASSEMBLED!\n";
            } else {
                std::cout << "-> Status: Still waiting for " << mydata.total_length - mydata.bytes_received << "...\n";
            }
            break;
        }
    }
}

int main() {
    std::cout << "--- SIMULATION START ---\n";

    // TEST 1: First Frame (Start of VIN Response)
    u8 frame1[] = {0x10, 0x0D, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF}; 
    process_frame(frame1);

    std::cout << "\n-------------------\n";

    // TEST 2: Consecutive Frame (End of VIN Response)
    u8 frame2[] = {0x21, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77};
    process_frame(frame2);

    return 0;
}
