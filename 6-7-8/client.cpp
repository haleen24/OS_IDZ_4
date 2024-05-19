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
    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
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
    if (connect(sock, (struct sockaddr*)&serv, sizeof(serv)) < 0) {
        std::cout << "connection() failed\n";
        exit(-1);
    }
    init_dict();
    std::string message = "worker";
    send(sock, message.c_str(), TASK_SIZE, 0);

    std::cout << "You are connected\n";

    for (;;) {
        auto task = recive_task(sock);
        if (task.id == -1) {
            close(sock);
            exit(0);
        }
        auto answer = encode(task.task);
        send_task(task.id, task.socket, answer, MSG_DONTWAIT);
        std::cout << "task with id: " << task.id << " were sent, answer: " << answer << "\n";
    }
}
