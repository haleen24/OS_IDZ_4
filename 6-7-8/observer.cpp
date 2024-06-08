#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstring>
#include <csignal>
#include "../lib.h"

int main(int argc, char** argv) {
    set_signal_handler();
    int sock;
    struct ip_mreq req;
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
    serv.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sock, (struct sockaddr*)&serv, sizeof(serv)) < 0) {
        Die("bind() failed");
    }

    req.imr_multiaddr.s_addr = inet_addr(serverIp);
    req.imr_interface.s_addr = htonl(INADDR_ANY);

    if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void*)&req,
                   sizeof(req)) < 0) {
        Die("setsockopt() failed");
    }
    init_dict();

    std::cout << "You are connected\n";

    for (;;) {
        auto log = recive_log(sock, 0);
        std::cout << log << '\n';
    }
}
