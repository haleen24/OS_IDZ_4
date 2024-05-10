#include "../lib.h"


int number_of_workers;
std::vector<struct sockaddr_in> clients;
std::vector<int> clients_sock;


void accept_clients(const int serv_sock) {
    for (int i = 0; i < number_of_workers; ++i) {
        char buff[TASK_SIZE];
        socklen_t clientSize = sizeof(clients[i]);
        auto &cl_sock = clients_sock[i];
        if ((cl_sock = accept(serv_sock, reinterpret_cast<struct sockaddr *>(&clients[i]), &clientSize)) < 0) {
            Die("accept() failed");
        }
        const size_t size = recv(cl_sock, buff, TASK_SIZE, 0);
        auto msg = std::string(buff, size);
        if (msg == "worker") {
            continue;
        }
    }
}

int main(int argc, char **argv) {
    if (argc != 5) {
        Die("ARG FORMAT: <SERVER IP, SERVER PORT, NUMBER OF WORKERS, MESSAGE>");
    }
    const char *server_ip = argv[1];
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
    if ((serv_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        Die("socket() failed");
    }

    if (bind(serv_sock, reinterpret_cast<struct sockaddr *>(&serv), sizeof(serv)) < 0) {
        Die("bind() failed");
    }
    std::cout << "Server on: " << inet_ntoa(serv.sin_addr) << ":" << port << '\n';
    if (listen(serv_sock, 2 * number_of_workers) < 0) {
        Die("listen() failed");
    } else {
        std::cout << "listening\n";
    }
    accept_clients(serv_sock);

    std::cout << "Workers are connected\n";

    init_dict();

    auto tasks = split(message);

    for (int i = 0; i < tasks.size(); ++i) {
        send_task(i, clients_sock[i % clients_sock.size()], tasks[i], MSG_DONTWAIT);
    }
    std::vector<TaskStruct> answers(tasks.size());
    for (int i = 0; i < tasks.size(); ++i) {
        answers[i] = recive_task(clients_sock[i % clients_sock.size()], 0);
    }
    std::cout << "all data were gotten\n";
    for (int i = 0; i < tasks.size(); ++i) {
        std::cout << "id: " << i << ", answer: " << answers[i].task << '\n';
    }
    for (auto &item: clients_sock) {
        close(item);
    }
}
