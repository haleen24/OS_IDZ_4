#include "../lib.h"


int number_of_workers;
std::vector<struct sockaddr_in> clients;
std::vector<int> clients_sock;


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
        if (msg == "worker") {
            continue;
        }
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
    clients = std::vector<sockaddr_in>(number_of_workers);
    clients_sock = std::vector<int>(number_of_workers);
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

    std::cout << "waiting for clients\n";
    accept_clients(serv_sock);

    std::cout << "Workers are connected\n";

    init_dict();

    auto tasks = split(message);

    for (int i = 0; i < tasks.size(); ++i) {
        send_task(i, serv_sock, tasks[i], 0, reinterpret_cast<sockaddr*>(&clients[i % clients.size()]),
                  sizeof(clients[i % clients.size()]));
    }
    std::vector<TaskStruct> answers(tasks.size());
    for (int i = 0; i < tasks.size(); ++i) {
        sockaddr_in addr;
        socklen_t t = sizeof(addr);
        TaskStruct answer = recive_task(serv_sock, 0, reinterpret_cast<sockaddr*>(&addr), &t);
        answers[answer.id] = std::move(answer);
        std::cout << "task witd id: " << answer.id << " were gotten\n";
    }
    std::cout << "all data were gotten\n";
    for (int i = 0; i < tasks.size(); ++i) {
        std::cout << "id: " << i << ", answer: " << answers[i].task << '\n';
    }
    for (auto& item : clients) {
        sendto(serv_sock, {}, 0, 0, reinterpret_cast<sockaddr*>(&item), sizeof(item));
    }
    for (auto& item : clients_sock) {
        close(item);
    }
}
