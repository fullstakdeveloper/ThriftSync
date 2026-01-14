#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#define PORT 8080

int main(int argc, char const* argv[]) {
    int status, valread, client_fd;
    struct sockaddr_in serv_addr;

    char* text = "hello this is client";
    char buffer[1024] = { 0 };

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

    send(client_fd, text, strlen(text), 0);
    printf("message sent to server!\n");

    valread = read(client_fd, buffer, 1024 - 1); 

    printf("%s\n", buffer);

    close(client_fd);
    return 0;


}