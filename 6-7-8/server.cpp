#include <list>
#include <queue>
#include <stack>

#include "../lib.h"


int number_of_workers;
pthread_mutex_t client_access_mutex;
std::vector<struct sockaddr_in> clients;
std::vector<int> clients_sock;
std::vector<struct sockaddr_in> listeners;
std::vector<int> listeners_sock;

void* send_log(void* msg_ptr) {
    const auto* param = static_cast<std::string*>(msg_ptr);
    std::string msg = "log: " + *param;
    delete param;
    if (msg.size() > LOG_SIZE) {
        msg = std::string(msg.begin(), msg.begin() + LOG_SIZE);
    }
    pthread_mutex_lock(&client_access_mutex);
    for (auto listener : listeners_sock) {
        if (send(listener, msg.c_str(), LOG_SIZE, MSG_DONTWAIT) < 0) {
            std::cout << "SERVER LOG: some troubles with sending observers' logs\n";
        }
    }
    pthread_mutex_unlock(&client_access_mutex);
    pthread_exit(nullptr);
}

void* accept_listeners(void* ptr) {
    // signal(SIGINT, sighandler);
    int serv_sock = *static_cast<int*>(ptr);
    for (;;) {
        listeners.push_back(sockaddr_in{});
        socklen_t clientSize = sizeof(listeners[listeners.size() - 1]);
        int sock;
        if ((sock = accept(serv_sock, reinterpret_cast<struct sockaddr*>(&listeners[listeners.size() - 1]),
                           &clientSize)) < 0) {
            listeners.pop_back();
            Die("accept() listeners failed");
        }
        pthread_mutex_lock(&client_access_mutex);
        listeners_sock.push_back(sock);
        char buff[TASK_SIZE];
        const size_t size = recv(sock, buff, TASK_SIZE, 0);
        auto msg = std::string(buff, size);
        if (msg == "observer") {
            std::cout << "observer connected\n";
        }
        else {
            std::cout << "Somebody has connected, but he is not an observer!\n";
            close(sock);
            listeners.pop_back();
        }
        pthread_mutex_unlock(&client_access_mutex);
    }
    return nullptr;
}

void accept_clients(const int serv_sock) {
    for (int i = 0; i < number_of_workers; ++i) {
        char buff[TASK_SIZE];
        socklen_t clientSize = sizeof(clients[i]);
        auto& cl_sock = clients_sock[i];
        if ((cl_sock = accept(serv_sock, reinterpret_cast<struct sockaddr*>(&clients[i]), &clientSize)) < 0) {
            Die("accept() failed");
        }
        const size_t size = recv(cl_sock, buff, TASK_SIZE, 0);
        auto msg = std::string(buff);
        if (msg == "worker") {
            continue;
        }
        if (msg == "observer") {
            listeners.push_back(clients[i]);
            listeners_sock.push_back(cl_sock);
        }
        --i;
    }
}

int main(int argc, char** argv) {
    pthread_mutex_init(&client_access_mutex, nullptr);
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
    if ((serv_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        Die("socket() failed");
    }

    if (bind(serv_sock, reinterpret_cast<struct sockaddr*>(&serv), sizeof(serv)) < 0) {
        Die("bind() failed");
    }
    std::cout << "Server on: " << inet_ntoa(serv.sin_addr) << ":" << port << '\n';
    if (listen(serv_sock, 2 * number_of_workers) < 0) {
        Die("listen() failed");
    }
    else {
        std::cout << "listening\n";
    }
    accept_clients(serv_sock);

    std::cout << "Workers are connected\n";

    pthread_t accepter;
    pthread_create(&accepter, nullptr, accept_listeners, (void*)&serv_sock);
    pthread_t tmp;
    std::string msg;
    std::vector<pthread_t> log_threads;
    auto tasks = split(message);
    for (int i = 0; i < tasks.size(); ++i) {
        const int cl_sock = clients_sock[i % clients_sock.size()];
        send_task(i, cl_sock, tasks[i], MSG_DONTWAIT);
        msg = "task " + tasks[i] + " was sent to worker (sock: " + std::to_string(cl_sock) + ")";
        pthread_create(&tmp, nullptr, send_log, new std::string(msg));
        log_threads.push_back(tmp);
    }
    std::vector<TaskStruct> answers(tasks.size());
    for (int i = 0; i < tasks.size(); ++i) {
        answers[i] = recive_task(clients_sock[i % clients_sock.size()], 0);
        msg = "task with id " + std::to_string(answers[i].id) + " was gotten (sock: " + std::to_string(
            answers[i].socket);
        pthread_create(&tmp, nullptr, send_log, new std::string(msg));
        log_threads.push_back(tmp);
    }
    msg = "all data were gotten";
    pthread_create(&tmp, nullptr, send_log, new std::string(msg));
    log_threads.push_back(tmp);
    std::cout << msg << '\n';
    for (int i = 0; i < tasks.size(); ++i) {
        msg = "id: " + std::to_string(i) + ", answer: " + answers[i].task;
        std::cout << msg << '\n';
        pthread_create(&tmp, nullptr, send_log, new std::string(msg));
        log_threads.push_back(tmp);
    }
    for (auto& item : clients_sock) {
        close(item);
    }
    pthread_cancel(accepter);
    msg = "exit";
    pthread_create(&tmp, nullptr, send_log, new std::string(msg));
    pthread_join(tmp, nullptr);
    pthread_mutex_lock(&client_access_mutex);
    log_threads.push_back(tmp);
    for (auto& item : log_threads) {
        pthread_cancel(item);
    }
    for (auto& item : listeners_sock) {
        close(item);
    }
    pthread_mutex_destroy(&client_access_mutex);
}
