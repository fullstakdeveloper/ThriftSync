# ZenithDrop: Secure P2P File Transfer for the Terminal
A low-level, high-throughput file transfer CLI built in C++ using the Berkeley Sockets API. ZenithDrop implements a custom binary protocol with AES-256 equivalent encryption, designed to stream massive payloads across local networks with a strict O(1) memory footprint.

## Project Overview
ZenithDrop moves files between machines as fast as possible while maintaining a tiny memory footprint and end-to-end encryption. By bypassing high-level abstractions, this project provides granular control over the TCP byte stream, ensuring data integrity and confidentiality for any file type (Text, Images, Video).

Think AirDrop — but for the Linux terminal.

### Key Technical Specs:
* Protocol: Custom Binary Header (268-byte fixed width)
* Encryption: XChaCha20-Poly1305 (AES-256 equivalent) via libsodium
* Key Exchange: Diffie-Hellman (crypto_kx) — shared secret never transmitted
* Memory Profile: O(1) Space Complexity (Fixed 1024-byte buffer)
* Architecture: Multi-threaded server with ThreadPool, sequential binary chunk streaming

## The Zenith Header
To solve the "Message Framing" problem in TCP, every transfer is preceded by a metadata header. This ensures the server knows exactly how many bytes to expect, preventing buffer overflows or desynchronization.

| Offset | Field | Type | Description |
| :--- | :--- | :--- | :--- |
| 0 | Version | uint32_t | Protocol versioning (Current: v1) |
| 4 | Type | uint32_t | Data ID (1: Text, 2: Image, 3: Video) |
| 8 | Payload_Size | uint32_t | Size of the original file in bytes |
| 12 | Filename | char[256] | Name of the file being transferred |

## Engineering Highlights

### 1. Constant Space Complexity (O(1))
ZenithDrop avoids the common pitfall of reading entire files into RAM. Instead, it utilizes a streaming loop:
1. Read exactly 1024 bytes from disk
2. Encrypt the chunk in place
3. Send encrypted bytes over the socket
4. Repeat until the full file is transferred

**Result:** The engine can transfer a 10GB file on a machine with minimal available RAM without performance degradation.

### 2. End-to-End Encryption
Every chunk is encrypted using XChaCha20-Poly1305 via libsodium's `crypto_secretstream` API. This provides:
* **Confidentiality** — data is unreadable in transit
* **Authentication** — tampered chunks are detected and rejected
* **Stream integrity** — out-of-order or replayed chunks are rejected

### 3. Diffie-Hellman Key Exchange
The encryption key is never transmitted over the wire. Instead, both sides perform a Diffie-Hellman exchange using libsodium's `crypto_kx` API, independently deriving the same shared secret. An attacker capturing all traffic cannot decrypt the data.

### 4. Binary-First Design
By utilizing `std::ios::binary`, the engine treats all data as raw bytes. This ensures JPEGs, MP4s, and PDFs are transferred bit-perfectly without the OS corrupting data through character encoding translations.

### 5. Multi-threaded Server
The server uses a custom ThreadPool implementation, handling multiple concurrent transfers without blocking.

## Getting Started

### Prerequisites
* GCC/G++ Compiler (C++17)
* POSIX-compliant OS (Linux/macOS)
* libsodium

### Installing libsodium
```bash
# macOS
brew install libsodium

# Ubuntu/Debian
sudo apt install libsodium-dev
