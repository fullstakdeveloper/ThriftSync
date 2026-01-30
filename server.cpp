#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdint.h>
#include <fstream>
#include <algorithm>
#include <thread>
#define PORT 8080

//this is the server file
//uint32_t ensures that it is 4 bytes on every system
struct ZenithHeader {
    uint32_t version; // idk protocol version
    uint32_t type; // 1 for text, 2 for image, 3 for video
    uint32_t payload_size; // the size of the data being sent
    std::string filename;
};

void handle_client(int client_socket) {

    //for constant memory usage
    char buffer[1024] = {0};

    //header structure
    ZenithHeader receivedHeader; 
    //getting the bytes from the cleints
    int headerBytesRead = recv(client_socket, &receivedHeader, sizeof(ZenithHeader), 0);

    //printing out the values from the client
    if (headerBytesRead == sizeof(ZenithHeader)) {
        printf("header received!\n");
        printf("version: %u\n", receivedHeader.version);
        printf("type: %u\n", receivedHeader.type);
        printf("payload size: %u\n", receivedHeader.payload_size);
    }

    //confirmation that header read back to client
    printf("header confimation sent\n");
    const char* ack = "header-received";
    send(client_socket, ack, strlen(ack), 0);

    //variables for O(1) memory usage
    int curr_bytes_received = 0;
    std::ofstream outFile("received_" + receivedHeader.filename, std::ios::binary);

    //reading bytes in chunks and sending to the backend
    while (curr_bytes_received < receivedHeader.payload_size) {
        printf("received");
        //had to type cast due to min specifications
        int bytes_to_read = std::min((uint32_t)1024, (uint32_t)(receivedHeader.payload_size - curr_bytes_received));
        int valread = recv(client_socket, buffer, bytes_to_read, 0);

        //logic for loop exist after all bytes obtained
        if (valread > 0) {
            outFile.write(buffer, valread);
            curr_bytes_received += valread;
            send(client_socket, "ok", 2, 0);
        } else if (valread == 0) {
            break; 
        } else {
            perror("recv failed");
            break;
        }

        //tracking
        printf("(%d, %d)\n", curr_bytes_received, receivedHeader.payload_size);
    }

    ack = "data received";
    send(client_socket, ack, strlen(ack), 0);
    close(client_socket);
}

int main(int argc, char const* agrv[]) {

    char buffer[1024] = {0};

    //AF_INET --> tells the computer how to find the other person(communication endpoint)
    //SOCK_STREAM --> tells the computer how the data should behave(continuous flow), SOCK_DGRAM is the alternate
    // 0 --> tells program to choose the most logical protocol(TCP)
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in address;

    //putting values in the sockaddr_in
    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);
    address.sin_addr.s_addr = INADDR_ANY;

    //bind
    //since the bind function bind was written for all address types
    //we have to type cast the sockaddr_in to sockaddr pointer
    //bind attaches the socket to a specific port and an IP address
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // if 3 peopel call in the same milisecond, listen will keep them in a waiting line
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("server is listening on port %d \n", PORT);

    int addrlen = sizeof(address);

    while(true) {
    //when the client finally tries to reach you, it basically calls accept which returns a socket that we use to talk to that other server
        int new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);

        if (new_socket >= 0) {
            printf("connection was successful\n");
            std::thread t(handle_client, new_socket);
            t.detach();
        }
    }

    // memset(buffer, 0, sizeof(buffer));
    // int valread = read(new_socket, buffer, 1024 - 1);
    

    // if (valread < 0) {
    //     perror("read failed");
    // } else {
    //     buffer[valread] = '\0';
    //     printf("received from client: %s\n", buffer);
    // }

    // close(new_socket);
    close(server_fd);
    close(server_fd);

    return 0;

    //add the abilty to have multiple connnections to the server

}
