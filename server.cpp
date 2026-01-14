#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#define PORT 8080

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

    //when the client finally tries to reach you, it basically calls accept which returns a socket that we use to talk to that other server
    int new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);

    if (new_socket < 0) {
        perror("accept failed");
        exit(EXIT_FAILURE);
        printf("connection was successful");
    }

    int valread = read(new_socket, buffer, 1024 - 1);

    if (valread < 0) {
        perror("Read failed");
    } else {
        printf("Received from client: %s\n", buffer);
    }

    close(new_socket);
    close(server_fd);

    return 0;

}