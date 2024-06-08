#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstring>
#include <csignal>
#include "../lib.h"

int main(int argc, char** argv) {
    set_signal_handler();
    int sock;
    if (argc != 3) {
        std::cout << "ARG FORMAT: <SERVER IP> <SERVER PORT>\n";
        exit(-1);
    }
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        std::cout << "socket() failed\n";
        exit(-1);
    }
    char* serverIp = argv[1];
    unsigned short serverPort = atoi(argv[2]);
    struct sockaddr_in serv{};
    memset(&serv, 0, sizeof(serv));
    serv.sin_port = htons(serverPort);
    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = inet_addr(serverIp);
    init_dict();
    std::string message = "worker";
    sendto(sock, message.c_str(), TASK_SIZE, 0, reinterpret_cast<sockaddr*>(&serv), sizeof(serv));
    std::cout << "You are ready\n";
    for (;;) {
        socklen_t t = sizeof(serv);
        auto task = recive_task(sock, 0, reinterpret_cast<sockaddr*>(&serv), &t);
        if (task.id == -1) {
            close(sock);
            exit(0);
        }
        auto answer = encode(task.task);
        send_task(task.id, sock, answer, 0, reinterpret_cast<sockaddr*>(&serv), sizeof(serv));
        std::cout << "The task with id: " << task.id << " were sent. answer is: " << answer << "\n";
    }
}
