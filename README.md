# Custom C++ ISO-TP Receiver Simulation

## Overview

This project is a compact C++ simulation of the receiver-side logic of the **ISO-TP protocol** (ISO 15765-2). It was developed to understand how multi-frame messages are transported over CAN and safely reassembled under embedded-style software constraints.

The implementation focuses on:
- Deterministic memory usage
- Manual protocol parsing
- Bounded payload assembly
- Avoiding dynamic allocation or external protocol libraries

---

## Objective

The goal of this project was to deepen my understanding of low-level communication software and embedded software constraints. In particular, I wanted to understand:

- How ISO-TP frames are structured
- How large messages are segmented and reassembled over CAN
- Why static memory allocation matters in constrained systems
- How safety-oriented coding practices improve predictability and robustness

---

## Implemented Features

This simulation implements the core receiver-side logic for:

- **Single Frame (SF)**
- **First Frame (FF)**
- **Consecutive Frame (CF)**

### Capabilities

The program:
- Manually parses the PCI byte using bitwise operations
- Extracts the total payload length from First Frames
- Stores payload data in a fixed static buffer
- Reassembles multi-frame payloads in bounded chunks
- Simulates Flow Control (CTS) generation
- Tracks reception state using explicit message-state variables

---

## Design Choices

### 1. Static Memory Allocation

A fixed **4096-byte** buffer is used for payload reassembly.

**Benefits:**
- No dynamic allocation
- No heap fragmentation
- Predictable memory behavior

### 2. Manual Protocol Parsing

Frame types are decoded directly from the PCI byte using bitwise operations. This approach:
- Builds lower-level understanding of ISO-TP transport behavior
- Avoids hiding logic behind existing libraries
- Provides educational value for embedded software concepts

### 3. Bounded Copying

Payload data is copied using bounded chunk sizes based on the number of remaining bytes:
- Reduces risk of copying beyond intended message length
- Ensures predictable memory operations

### 4. MISRA-Inspired Practices

The implementation uses several safety-oriented practices commonly associated with embedded development:
- Fixed-width integer types (`uint8_t`, `uint16_t`)
- Explicit narrowing conversions with `static_cast`
- Named constants instead of magic numbers
- Explicit state reset and status handling

---

## File Structure
```
.
└── isotp_receiver.cpp    Main source file containing the ISO-TP receiver simulation
```

---

## Build and Run

### Compile
```bash
g++ isotp_receiver.cpp -o ecu_sim
```

### Run
```bash
./ecu_sim
```

---

## Sample Output
```
--- ISO-TP RECEIVER SIMULATION START ---
[Detected] FIRST FRAME
-> Total message size: 13 bytes
-> Captured initial 6 bytes
-> Status: WAITING (7 bytes remaining)
   [TX] Sending FLOW CONTROL (CTS)
   [TX] Block Size: 0 (Send all remaining frames)
   [TX] STmin: 10 ms
----------------------------------------
[Detected] CONSECUTIVE FRAME
-> Sequence number: 1
-> Status: COMPLETE MESSAGE ASSEMBLED
--- SIMULATION END ---
```

---

## Current Limitations

This project is a simulation and not a complete production-ready ISO-TP stack.

### Known Limitations

- Flow Control is simulated and printed, not transmitted through a real CAN interface
- Sequence number validation for Consecutive Frames is not implemented
- Timeout handling is not implemented
- The code demonstrates transport-layer receiver logic only
- No real CAN driver or SocketCAN integration is included in this version

---

## Why This Project Matters

Although compact, this project demonstrates software engineering concepts that are relevant to embedded and systems programming:

- Protocol-aware software design
- Deterministic memory usage
- Explicit state management
- Safety-conscious data handling
- Software behavior under communication constraints

---

## Possible Extensions

Future improvements could include:

- Sequence number validation
- Timeout and error recovery logic
- SocketCAN or virtual CAN integration
- Sender-side ISO-TP support
- Extension toward a more complete diagnostic communication stack

---

## Author

**Yassine Ezzahy**
