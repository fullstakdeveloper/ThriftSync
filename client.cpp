#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdint.h>
#include <fstream>
#include <vector>
#include <iostream>
#include <fstream>
#include <vector>
#include <iostream>
#include <sodium.h>
#define PORT 8080

//to compile
//g++ -std=c++17 -o client client.cpp $(pkg-config --cflags --libs libsodium)

//This is the Client File:
//1. It takes local data and send it to the server using chunked streaming
//3. It send data by encrypting the raw bytes first and creates the key

struct ZenithHeader {
    uint32_t version; 
    uint32_t type; 
    uint32_t payload_size; 
    char filename[256];
};

//main function
int main(int argc, char const* argv[]) {

    //for encryption
    if (sodium_init() < 0) {
        printf("libsodium init failed\n");
        return -1;
    }

    //makes sure athe arguments needed to run and received
    if (argc != 3) {
        printf("usage: zenithdrop <ip> <filepath>\n");
        return -1;
    }

    //defining usable variables from command input
    const char* target_ip = argv[1];
    const char* filepath  = argv[2];
    
    
    int status, valread, client_fd;
    struct sockaddr_in serv_addr;

    //creating client socket
    client_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (client_fd < 0) {
        printf("client socket failed\n");
        return -1;
    }
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    //establishing connection to the server
    if (inet_pton(AF_INET, target_ip, &serv_addr.sin_addr) <= 0) {
        printf("invalid address\n");
        return -1; 
    }

    status = connect(client_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    
    if (status < 0) {
        printf("connection failed\n");
        return -1;
    }

    //create the file transfre header for meta data information
    ZenithHeader header;

    strncpy(header.filename, filepath, sizeof(header.filename) - 1);
    std::ifstream inFile(header.filename, std::ios::binary);

    if (!inFile){
        perror("Could not open file");
        close(client_fd);
        return -1;
    }

    inFile.seekg(0, std::ios::end);
    
    header.version = 1;
    header.type = 1;
    header.payload_size = (uint32_t)inFile.tellg();

    
    //file meta data confirmation
    send(client_fd, &header, sizeof(ZenithHeader), 0);
    printf("header sent to server!\n");
    printf("sent ... waiting ...\n");

    char ack_buffer[1024];
    memset(ack_buffer, 0, 1024);

    valread = 0;

    //reads for server confirmation
    while (valread <= 0) {
        valread = read(client_fd, ack_buffer, 1024 - 1);
    }

    if (valread > 0) {
        if (strstr(ack_buffer, "header-received") != 0) {
            printf("handshake done ... starting shovel...\n");
        } else {
            printf("handshake failed %s\n", ack_buffer);
        }
    }

    inFile.clear();         
    inFile.seekg(0, std::ios::beg);

    //receiving server public key
    unsigned char server_pk[crypto_kx_PUBLICKEYBYTES];
    recv(client_fd, server_pk, sizeof(server_pk), 0);

    //generate the client keypair
    unsigned char client_pk[crypto_kx_PUBLICKEYBYTES];
    unsigned char client_sk[crypto_kx_SECRETKEYBYTES];
    crypto_kx_keypair(client_pk, client_sk);

    //send the client public key to server
    send(client_fd, client_pk, sizeof(client_pk), 0);

    //creating the shared secrets
    unsigned char rx[crypto_kx_SESSIONKEYBYTES];
    unsigned char tx[crypto_kx_SESSIONKEYBYTES];

    if (crypto_kx_client_session_keys(rx, tx, client_pk, client_sk, server_pk) != 0) {
        printf("key exchange failed\n");
        close(client_fd);
        return -1;
    }

    //encryption header send and verification
    crypto_secretstream_xchacha20poly1305_state state;
    unsigned char encrypt_header[crypto_secretstream_xchacha20poly1305_HEADERBYTES];

    crypto_secretstream_xchacha20poly1305_init_push(&state, encrypt_header, tx);
    send(client_fd, encrypt_header, sizeof(encrypt_header), 0);
    
    //buffer for confirmation message
    char conf_buffer[16] = {0};
    char send_buffer[1024];

    //buffer for sending
    unsigned char cipher_buffer[1024 + crypto_secretstream_xchacha20poly1305_ABYTES];
    unsigned long long cipher_len;

    while (inFile.read(send_buffer, sizeof(send_buffer)) || inFile.gcount() > 0) {
        bool sent = false;

        while (!sent) {
            crypto_secretstream_xchacha20poly1305_push(&state,cipher_buffer, &cipher_len, (unsigned char*)send_buffer, inFile.gcount(), NULL, 0,
            crypto_secretstream_xchacha20poly1305_TAG_MESSAGE
            );

            send(client_fd, cipher_buffer, cipher_len, 0);

            recv(client_fd, conf_buffer, sizeof(conf_buffer), 0);
        
            if (strstr(conf_buffer, "ok")) {
                sent = true; 
                printf("{properly sent}\n");
            } else {
                printf("{retrying shovel}\n");
            }
        }

    }

    printf("Sent! Waiting...\n");

    valread = read(client_fd, ack_buffer, 1024 - 1); 
    printf("%s\n", ack_buffer);

    printf("client closed");
    
    close(client_fd);
    return 0;
}
