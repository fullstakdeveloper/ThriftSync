#include <sodium.h>
#include <stdio.h>
#include <iostream>
#include "client.h"
#include "server.h"
#include "protocol.h"

//to compile
//g++ -std=c++17 -pthread -o zenithdrop main.cpp client.cpp server.cpp $(pkg-config --cflags --libs libsodium)

int main(int argc, char const* argv[]) {

    //check for correct arugument 
    if (argc < 2) {
        printf("usage: zenithdrop <send|receive|share> [args]\n");
        return -1;
    }

    //for encryption
    if (sodium_init() < 0) {
        printf("libsodium init failed\n");
        return -1;
    }

    std::string type = argv[1];

    if (type == "send") {
        if (argc != 4) {
            printf("usage: zenithdrop send <ip> <filepath>\n");
            return -1;
        }
        return run_client(argv[2], argv[3]);

    } else if (type == "receive") {
        return run_server();

    } else if (type == "share") {
        // QR code + HTTP server — coming soon

    } else {
        printf("unknown command: %s\n", argv[1]);
        return -1;
    }
}