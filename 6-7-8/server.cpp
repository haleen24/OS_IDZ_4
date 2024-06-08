#include <cmath>

#include "../lib.h"


int number_of_workers;
std::vector<struct sockaddr_in> clients;

struct sockaddr_in multicast_addr;
int multicast_sock;

void send_log(std::string log) {
    if (log.size() > LOG_SIZE) {
        log = std::string(log.begin(), log.begin() + LOG_SIZE);
    }
    sendto(multicast_sock, log.c_str(), log.size(), 0, reinterpret_cast<sockaddr*>(&multicast_addr),
           sizeof(multicast_addr));
}

void accept_clients(const int serv_sock) {
    for (int i = 0; i < number_of_workers; ++i) {
        char buff[TASK_SIZE];
        auto& client_addr = clients[i];
        size_t size;
        socklen_t len = sizeof(client_addr);
        if ((size = recvfrom(serv_sock, buff, TASK_SIZE, 0, reinterpret_cast<sockaddr*>(&client_addr), &len)) == 0) {
            Die("cant get start message from client");
        }
        auto msg = std::string(buff, size);
    }
}

int main(int argc, char** argv) {
    if (argc != 5) {
        Die("ARG FORMAT: <SERVER IP, SERVER PORT, NUMBER OF WORKERS, MESSAGE>");
    }
    const char* server_ip = argv[1];
    unsigned short port = atoi(argv[2]);
    number_of_workers = atoi(argv[3]);
    if (number_of_workers <= 0) {
        Die("Incorrect number of workers");
    }
    char* multicast_ip = "224.0.0.0";
    unsigned short multicast_port = 8081;
    unsigned char multicast_ttl = 100;
    if ((multicast_sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        Die("cant use multicast");
    }
    if (setsockopt(multicast_sock, IPPROTO_IP, IP_MULTICAST_TTL, (void*)&multicast_ttl,
                   sizeof(multicast_ttl)) < 0) {
        Die("setsockopt() failed");
    }
    memset(&multicast_addr, 0, sizeof(multicast_addr));
    multicast_addr.sin_family = AF_INET;
    multicast_addr.sin_addr.s_addr = inet_addr(multicast_ip); /* Multicast IP address */
    multicast_addr.sin_port = htons(multicast_port);

    clients = std::vector<sockaddr_in>(number_of_workers);
    auto message = std::string(argv[4]);
    int serv_sock;
    struct sockaddr_in serv{};
    memset(&serv, 0, sizeof(serv));
    serv.sin_addr.s_addr = inet_addr(server_ip);
    serv.sin_port = htons(port);
    serv.sin_family = AF_INET;
    if ((serv_sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        Die("socket() failed");
    }

    if (bind(serv_sock, reinterpret_cast<struct sockaddr*>(&serv), sizeof(serv)) < 0) {
        Die("bind() failed");
    }
    std::cout << "Server on: " << inet_ntoa(serv.sin_addr) << ":" << port << '\n';

    send_log("server waiting for clients...");

    std::cout << "waiting for clients\n";

    accept_clients(serv_sock);

    send_log("all clients are connected");

    std::cout << "Workers are connected\n";

    init_dict();

    auto tasks = split(message);

    for (int i = 0; i < tasks.size(); ++i) {
        send_task(i, serv_sock, tasks[i], 0, reinterpret_cast<sockaddr*>(&clients[i % clients.size()]),
                  sizeof(clients[i % clients.size()]));
        send_log("task with id " + std::to_string(i) + " were sent");
    }
    std::vector<TaskStruct> answers(tasks.size());
    for (int i = 0; i < tasks.size(); ++i) {
        sockaddr_in addr;
        socklen_t t = sizeof(addr);
        TaskStruct answer = recive_task(serv_sock, 0, reinterpret_cast<sockaddr*>(&addr), &t);
        answers[answer.id] = std::move(answer);
        send_log("answer with id: " + std::to_string(answer.id) + " were gotten");
        std::cout << "task witd id: " << answer.id << " were gotten\n";
    }
    std::cout << "all data were gotten\n";
    send_log("all answers were gotten");
    send_log("output in correct order->");
    for (int i = 0; i < tasks.size(); ++i) {
        std::cout << "id: " << i << ", answer: " << answers[i].task << '\n';
        send_log("id: " + std::to_string(i) + ", answer: " + answers[i].task);
    }
    for (auto& item : clients) {
        sendto(serv_sock, {}, 0, 0, reinterpret_cast<sockaddr*>(&item), sizeof(item));
    }
    send_log("The end");
    send_log("");
}
