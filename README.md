# Custom C++ ISO-TP (UDS) Receiver Stack

## Overview
This project is a lightweight, static-memory implementation of the ISO-TP (ISO 15765-2) protocol, designed for embedded automotive systems (ECUs).

Unlike standard Linux implementations that rely on dynamic memory (`std::vector`), this project uses a **zero-copy, static buffer approach** to ensure deterministic behavior and memory safety, making it suitable for bare-metal microcontrollers or RTOS environments.

## Key Features
* **Memory Safety:** Uses strict bounds checking (`chunk` logic) to prevent buffer overflows during multi-frame reassembly.
* **Static Allocation:** No dynamic heap allocation (`new`/`malloc`), preventing memory fragmentation.
* **Bitwise Protocol Decoding:** Manually parses CAN frame headers to identify Frame Types (Single, First, Consecutive).
* **Pointer Arithmetic:** Uses efficient pointer offsets for data assembly instead of costly array copying loops.

## How to Run
1. Compile with any C++ compiler: 
   ```bash
   g++ src/isotp_receiver.cpp -o ecu_sim
2. Run the executable:
Bash
./ecu_sim
