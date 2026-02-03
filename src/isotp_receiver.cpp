#include<cstdint>
#include<iostream>
#include<cstring>
#include<iomanip>
using u8 =uint8_t;
using u16 =uint16_t;

struct data{
    u8 buffer[4096];
    u16 bytes_received;
    u16 total_length; 
    bool message_complete;
};

static data mydata;
// SIMULATION of sending a CAN Frame back to the Tester
void send_flow_control() {
    // FRAME CONSTRUCTION
    // Byte 0: PCI (0x30 = Flow Control type, 0x00 = Clear To Send status)
    //         Combined: 0x30
    // Byte 1: Block Size (BS). 0x00 = Send ALL remaining frames without stopping.
    // Byte 2: STmin (Separation Time). 0x0A = 10 milliseconds between frames.
    // Bytes 3-7: Unused/Padding (0x55 or 0x00 usually)
    
    u8 fc_frame[] = {0x30, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00};

    std::cout << "   [TX] << SENDING FLOW CONTROL (CTS)\n";
    std::cout << "   [TX]    BlockSize: 0 (Send All)\n";
    std::cout << "   [TX]    STmin:     10ms\n";
    
    // In Real Hardware: CAN_Transmit(0x7E8, fc_frame, 8);
}

void process_frame(const u8 *ptr){
u8 byte0= *ptr &0xFF;
u8 type=byte0>>4;

switch (type){
    case 0x00:{
        std::cout << "[detected] SINGLE FRAME (Length < 8)\n";
        u8 leng =byte0 & 0x0F;
        std::cout<<"sending a frame that have "<<(int)leng<<" bytes\n";
        mydata.total_length = leng;
        std::memcpy(mydata.buffer,&ptr[1],leng);
        mydata.bytes_received=leng;
        mydata.message_complete = true;
        std::cout << "-> Captured " << (int)mydata.bytes_received << " bytes.\n";
        std::cout << "-> Status: COMPLETE.\n";
        break;
    }
    case 0x01:{
        std::cout << "\n[Detected] FIRST FRAME (Type 1)\n";
        u16 requested_len = ((ptr[0] & 0x0F) << 8) | ptr[1];
        if (requested_len > 4096) {
            std::cout << "ERROR: Message too large (" << requested_len << " bytes)! \n";
            std::cout << "Buffer Limit is 4096. Ignoring this request.\n";
            return; 
            }
         mydata.bytes_received=0;
         mydata.message_complete = false;
         mydata.total_length=requested_len;
         std::cout << "-> Total Message Size will be: " << (int)mydata.total_length << " bytes.\n";
         std::memcpy(mydata.buffer,&ptr[2],6);
         mydata.bytes_received=6;
         std::cout << "-> Captured initial 6 bytes.\n";
          std::cout << "-> Status: WAITING (Need " << (mydata.total_length - 6) << " more bytes).\n";
          send_flow_control();

         break;}
    case 0x02:{
        std::cout << "\n[Detected] CONCECUTIVEFRAME (Type 2)\n";
        u16 remaining= mydata.total_length-mydata.bytes_received;
        u8 chunk;
        if(remaining>7){
            chunk=7;
        }
        else {chunk= static_cast<u8>(remaining);}
        std::memcpy(&mydata.buffer[mydata.bytes_received],&ptr[1],chunk);
        mydata.bytes_received+=chunk;

        if(mydata.bytes_received>=mydata.total_length){
            mydata.message_complete =true;
            std::cout << "-> Status: COMPLETE MESSAGE ASSEMBLED!\n";

        }
        else{
            std::cout << "-> Status: Still waiting for"<<mydata.total_length-mydata.bytes_received <<"...\n";
        }


        break;}


}


}
int main() {
    std::cout << "--- SIMULATION START ---\n";

   
    // "I am sending 13 bytes. Here are the first 6: AA BB CC DD EE FF"
    u8 frame1[] = {0x10, 0x0D, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF}; 
    process_frame(frame1);

    std::cout << "\n-------------------\n";

   
    // "Here are the remaining 7 bytes: 11 22 33 44 55 66 77"
    u8 frame2[] = {0x21, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77};
    process_frame(frame2);

    return 0;
}
