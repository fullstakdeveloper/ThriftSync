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
#define PORT 8080

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


    int status, valread, client_fd;
    struct sockaddr_in serv_addr;

    client_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (client_fd < 0) {
        printf("client socket failed\n");
        return -1;
    }
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("invalid address\n");
        return -1; 
    }

    status = connect(client_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    
    if (status < 0) {
        printf("connection failed\n");
        return -1;
    }

    ZenithHeader header;

    printf( "Enter the file name that you want to send");
    std::cin >> header.filename;

    std::ifstream inFile(header.filename, std::ios::binary);

    if (!inFile){
        perror("Could not open file");
    }

    inFile.seekg(0, std::ios::end);
    
    header.version = 1;
    header.type = 1;
    header.payload_size = (uint32_t)inFile.tellg();

    
    // send(client_fd, &header, sizeof(header), 0);
    send(client_fd, &header, sizeof(ZenithHeader), 0);
    printf("header sent to server!\n");
    printf("sent ... waiting ...\n");

    sleep(1);

    char ack_buffer[1024];
    memset(ack_buffer, 0, 1024);

    while (valread <= 0) {
        valread = read(client_fd, ack_buffer, 1024 - 1);
    } 

    if (valread > 0) {
        if (strstr(ack_buffer, "header-received") == 0) {
            printf("handshake done ... starting shovel...\n");
        } else {
            printf("handshake failed %s\n", ack_buffer);
        }
    }
    
    inFile.clear();         
    inFile.seekg(0, std::ios::beg);

    char conf_buffer[16] = {0};
    char send_buffer[1024];

    while (inFile.read(send_buffer, sizeof(send_buffer)) || inFile.gcount() > 0) {
        int bytes_read = inFile.gcount();
        bool sent = false;

        while (!sent) {
            send(client_fd, send_buffer, inFile.gcount(), 0);

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
