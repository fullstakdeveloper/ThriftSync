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

//what the hell is going on in this file, why are there so many bugs in this file

//this is the client file
//the merge is fixed and ready to go
//2. get the multi threading done with only 4 instances allowed
//3. then get encription and data integrity done

//returning the vector array(I need to keep O(1) memory on the client side)
// std::vector<char> imageToBytes(std::string filename) {
//     std::ifstream(filename, std:ios::binary)

// }

//uint32_t ensures that it is 4 bytes on every system
struct ZenithHeader {
    uint32_t version; // idk protocol version
    uint32_t type; // 1 for text, 2 for image, 3 for video
    uint32_t payload_size; // the size of the data being sent
    std::string filename = "";
};

//main function
int main(int argc, char const* argv[]) {
    int status, valread, client_fd;
    struct sockaddr_in serv_addr;

    //the buffer is updated allocate the ram used after getting data from server 
    //or the other way around, it is also used in the server file

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

    //until this point a connection to the other socket has been established
    //have seperate functions for sending text, image, and video
    //change the header for the zenith to accomodate for the file size

    ZenithHeader header;

    printf( "Enter the file name that you want to send");
    std::cin >> header.filename;

    std::ifstream inFile(header.filename, std::ios::binary);
    if (!inFile){perror("Could not open file");}

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
    
    //sends the cursor to the end of the of the file
    inFile.clear();         
    inFile.seekg(0, std::ios::beg);

    //shoveling process and sending
    char conf_buffer[16] = {0};
    char send_buffer[1024];
    //main loop for sending data in O(1);
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
