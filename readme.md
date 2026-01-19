# Zenith Protocol: High-Performance C++ Socket Engine

A low-level, high-throughput networking engine built in C++ using the Berkeley Sockets API. Zenith implements a custom application-layer protocol designed to handle massive binary data streams with constant memory overhead.



## Project Overview
The goal of Zenith is to move data from Client to Server as fast as possible while maintaining a tiny memory footprint. By bypassing high-level abstractions, this project provides granular control over the TCP byte stream, ensuring data integrity for any file type (Text, Images, Video).

### Key Technical Specs:
* Protocol: Custom Binary Header (12-byte fixed width).
* Memory Profile: O(1) Space Complexity (Fixed 1024-byte buffer).
* Architecture: Sequential binary "shoveling" for stable memory usage.

## The Zenith Header
To solve the "Message Framing" problem in TCP, every packet is preceded by a 12-byte metadata "label." This ensures the server knows exactly how many bytes to expect, preventing buffer overflows or desynchronization.

| Offset | Field | Type | Description |
| :--- | :--- | :--- | :--- |
| 0 | Version | uint32_t | Protocol versioning (Current: v1). |
| 4 | Type | uint32_t | Data ID (1: Text, 2: Image, 3: Video). |
| 8 | Payload_Size | uint32_t | Size of the data following the header. |



## Engineering Highlights

### 1. Constant Space Complexity (O(1))
Zenith avoids the common pitfall of reading entire files into RAM. Instead, it utilizes a streaming loop:
1. Read exactly 1024 bytes from the socket.
2. Write those 1024 bytes directly to the disk using std::ofstream.
3. Repeat until Payload_Size is reached.

**Result:** The engine can transfer a 10GB file on a machine with minimal available RAM without performance degradation.

### 2. Binary-First Design
By utilizing std::ios::binary, the engine treats all data as raw bytes. This ensures that JPEGs, MP4s, and PDFs are transferred bit-perfectly without the operating system corrupting data through unintended character encoding translations.



## Getting Started

### Prerequisites
* GCC/G++ Compiler
* POSIX-compliant OS (Linux/macOS)

### Installation & Execution
```bash
# 1. Clone the repository
git clone [https://github.com/fullstakdeveloper/zenith-socket](https://github.com/fullstakdeveloper/zenith-socket)

# 2. Compile the binaries
g++ server.cpp -o server
g++ client.cpp -o client

# 3. Start the server
./server

# 4. Run the client (in a new terminal)
./client
