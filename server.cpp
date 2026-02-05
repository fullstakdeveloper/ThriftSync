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
#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#define PORT 8080

void handle_client(int client_socket);

//this is the server file
//this is the threadpool class
class ThreadPool {
private:
  std::vector<std::thread> threads_;
  std::queue<int> tasks_;
  std::mutex que_mutex_;
  std::condition_variable cv_;
  bool stop_ = false;

public:
  //initializing the constructor for the class/there needs to a cue that allocates the tasks to the threads
  //there will be a total of 4 threads total
  ThreadPool(size_t num_threads = std::thread::hardware_concurrency()) {
    for (size_t i = 0; i < num_threads; i++) {
      threads_.emplace_back([this] {
        while(true) {
        int client_fd;
          {
          int socket;
          std::unique_lock<std::mutex> lock(this->que_mutex_);
          this->cv_.wait(lock, [this] {return this->stop_ || !this->tasks_.empty();});
          if (this->stop_ && this->tasks_.empty()) return;
          socket = this->tasks_.front();
          this->tasks_.pop();
          }

          handle_client(client_fd);
           
        }
      });
    }
  }


  void enqueue(int socket) {
     {
    std::unique_lock<std::mutex> lock(std::mutex);
    tasks_.push(socket);
    }
    cv_.notify_one();
  }

  ~ThreadPool() {
    {
      std::unique_lock<std::mutex> lock(que_mutex_);
      stop_ = true;
    }
    cv_.notify_all();
    for (std::thread &worker : threads_) worker.join();
  }


};
//uint32_t ensures that it is 4 bytes on every system
struct ZenithHeader {
    uint32_t version; //this is the just
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
            printf("a request was recevied\n");
            pool.enqueue(new_socket);
            cv_.notify_one();
        }
    }

    return 0;

}
