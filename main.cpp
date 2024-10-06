
#include <array>
#include <vector>
#include <iostream>
#include <liburing.h>
#include <netinet/in.h>
#include <string_view>
#include <sys/socket.h>
#include <arpa/inet.h>


int main(int argument_count, char** argument_value) {

    struct io_uring ring;
    if (io_uring_queue_init(4096, &ring, 0) != 0) {
        std::cout << "Failed to init io_uring" << "\n";
    }

    int socketFD = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in serverAddress = {};
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddress.sin_port = htons(5000);
    serverAddress.sin_family = AF_INET;

    if (bind(socketFD, (struct sockaddr*) &serverAddress, sizeof(serverAddress)) != 0) {
        std::cout << "Failed to bind server socket to a port" << "\n";
    }

    std::cout << "Creating buffers" << "\n";
    std::vector<std::array<char, 512>> buffers = {};
    for(int i = 0; i < 10; i++) {
        std::array<char, 512> buffer = {};
        buffers.push_back(buffer);
    }

    std::cout << "Creating SQE's" << "\n";
    for (int i = 0; i < 10; i++) {
        struct io_uring_sqe* sqe; 
        sqe = io_uring_get_sqe(&ring);
        const std::array<char, 512> buffer = buffers[i];
        sqe->user_data = (__u64) &buffer;
        io_uring_prep_recv(sqe, socketFD, (void*) buffer.data(), 512, 0);
        io_uring_submit(&ring);
    }

    std::cout << "Awaiting CQE's" << "\n";
    struct io_uring_cqe* cqe;

    while (true) {
        int errorCode = io_uring_wait_cqe(&ring, &cqe);
        if (errorCode != 0) {
            std::cout << "CQE wait failed with error: " << - errorCode << "\n";
        }

        std::array<char, 512> messageBuffer = *((std::array<char, 512> *) cqe->user_data);
        std::cout << "Message: " << std::string_view(messageBuffer.data(), (cqe->res) - 1) << "\n";

        io_uring_cqe_seen(&ring, cqe);
        // Give back SQE to ring buffer:
        struct io_uring_sqe* sqe; 
        sqe = io_uring_get_sqe(&ring);
        const std::array<char, 512> buffer = messageBuffer;
        sqe->user_data = (__u64) &buffer;
        io_uring_prep_recv(sqe, socketFD, (void*) buffer.data(), 512, 0);
        io_uring_submit(&ring);
    }

    io_uring_queue_exit(&ring);

    return 0;
}