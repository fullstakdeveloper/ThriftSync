#include <ifaddrs.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string>
#include <sodium.h>
#include <qrencode.h>
#include <fstream>
#include <unistd.h>

//basically traverses through a linked list and determines the devices ip address
std::string get_local_ip() {
    struct ifaddrs* ifaddr;
    char ip[INET_ADDRSTRLEN];

    getifaddrs(&ifaddr);

    for (struct ifaddrs* ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) {continue;}
        if (ifa->ifa_addr->sa_family != AF_INET) {continue;}
        if (strcmp(ifa->ifa_name, "lo") == 0) {continue;}

        struct sockaddr_in* addr = (struct sockaddr_in*)ifa->ifa_addr;
        inet_ntop(AF_INET, &addr->sin_addr, ip, sizeof(ip));
        freeifaddrs(ifaddr);
        return std::string(ip);
    }

    freeifaddrs(ifaddr);
    return "127.0.0.1";
}

int run_share(const char* filepath) {
    // get the machines ip address
    std::string ip = get_local_ip();
    
    // generate random token to make it more secure
    unsigned char token_bytes[8];
    randombytes_buf(token_bytes, sizeof(token_bytes));

    char token[17];

    for (int i = 0; i < 8; i++) {
        snprintf(token + i * 2, 3, "%02x", token_bytes[i]);
    }

    // building the url
    std::string filename = std::string(filepath).substr(std::string(filepath).find_last_of("/\\") + 1);

    std::string url = "http://" + ip + ":8080/" + filename + "?token=" + token;

    printf("url: %s\n", url.c_str());

    QRcode* qr = QRcode_encodeString(url.c_str(), 0, QR_ECLEVEL_L, QR_MODE_8, 1);

    if (qr == NULL) {
        printf("QR code generation failed\n");
        return -1;
    }

    for (int y = 0; y < qr->width; y++) {
        for (int x = 0; x < qr->width; x++) {
            if (qr->data[y * qr->width + x] & 1) {
                printf("██");
            } else {
                printf("  ");
            }
        }
        printf("\n");
    }

    QRcode_free(qr);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(8080);
    address.sin_addr.s_addr = INADDR_ANY;

    bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    listen(server_fd, 1);
    printf("waiting for phone to connect...\n");

    int addrlen = sizeof(address);
    int client_fd = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);

    char request[2048] = {0};
    read(client_fd, request, sizeof(request) - 1);

    if (strstr(request, token) == NULL) {
        printf("invalid token — rejected\n");
        close(client_fd);
        close(server_fd);
        return -1;
    }

    std::ifstream inFile(filepath, std::ios::binary);
    inFile.seekg(0, std::ios::end);
    long file_size = inFile.tellg();
    inFile.seekg(0, std::ios::beg);

    std::string headers = "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/octet-stream\r\n"
        "Content-Length: " + std::to_string(file_size) + "\r\n"
        "Content-Disposition: attachment; filename=\"" + filename + "\"\r\n"
        "\r\n";
    send(client_fd, headers.c_str(), headers.size(), 0);

    char send_buffer[1024];
    while (inFile.read(send_buffer, sizeof(send_buffer)) || inFile.gcount() > 0) {
        send(client_fd, send_buffer, inFile.gcount(), 0);
    }

    printf("file sent!\n");
    close(client_fd);
    close(server_fd);

    return 0;
}