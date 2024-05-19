#include <algorithm>
#include <cmath>
#include <list>
#include <queue>
#include <stack>
#include <unordered_set>
#include <utility>
#include <sys/epoll.h>
#include "../lib.h"

pthread_mutex_t client_access_mutex;
pthread_mutex_t answer_access_mutex;
pthread_mutex_t log_mutex;
pthread_cond_t empty_lock;
std::vector<struct sockaddr_in> clients;
std::vector<int> clients_sock;
std::vector<struct sockaddr_in> listeners;
std::vector<int> listeners_sock;
std::unordered_set<std::string> tasks{};
std::vector<TaskStruct> answers(tasks.size());
std::vector<pthread_t> log_threads;

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

pthread_t send_log_async(std::string msg) {
    pthread_t tmp;
    pthread_create(&tmp, nullptr, send_log, new std::string(std::move(msg)));
    return tmp;
}

void* accept_all(void* ptr) {
    struct epoll_event ev;
    int serv_sock = *static_cast<int*>(ptr);
    for (;;) {
        sockaddr_in addr{};
        socklen_t addr_size = sizeof(addr);
        int sock;
        if ((sock = accept(serv_sock, reinterpret_cast<struct sockaddr*>(&addr),
                           &addr_size)) < 0) {
            Die("accept() listeners failed");
        }
        pthread_mutex_lock(&client_access_mutex);
        char buff[TASK_SIZE];
        const size_t size = recv(sock, buff, TASK_SIZE, 0);
        auto msg = std::string(buff);
        if (msg == "observer") {
            std::cout << "observer connected\n";
            listeners_sock.push_back(sock);
            listeners.push_back(addr);
        }
        else if (msg == "worker") {
            std::cout << "client connected\n";
            clients_sock.push_back(sock);
            clients.push_back(addr);
        }
        else {
            std::cout << "Somebody has connected, but he is not an observer or worker!\n";
            close(sock);
        }
        if (!clients_sock.empty()) {
            pthread_cond_signal(&empty_lock);
        }
        pthread_mutex_unlock(&client_access_mutex);
    }
    return nullptr;
}

void* get_answers_thread(void* ptr) {
    auto answer = recive_task(*static_cast<int*>(ptr), MSG_NOSIGNAL);
    if (answer.id == -1) {
        return nullptr;
    }
    pthread_mutex_lock(&answer_access_mutex);
    if (answers[answer.id].id == -1) {
        answers[answer.id] = answer;
        auto msg = "answer " + answer.task + " was gotten (id = " + std::to_string(answer.id) + ")";
        std::cout << msg << '\n';
        pthread_mutex_lock(&log_mutex);
        auto tmp = send_log_async(msg);
        log_threads.push_back(tmp);
        pthread_mutex_unlock(&log_mutex);
    }
    pthread_mutex_unlock(&answer_access_mutex);
    return nullptr;
}

void* get_answers(void* ptr) {
    for (;;) {
        std::vector<pthread_t> threads{};
        pthread_mutex_lock(&client_access_mutex);
        for (int i = 0; i < clients_sock.size(); ++i) {
            pthread_t tmp;
            pthread_create(&tmp, nullptr, get_answers_thread, &clients_sock[i]);
            threads.push_back(tmp);
        }
        pthread_mutex_unlock(&client_access_mutex);
        for (const auto& tmp : threads) {
            pthread_join(tmp, nullptr);
        }
    }
}

bool check_end() {
    pthread_mutex_lock(&answer_access_mutex);
    const bool answer = std::all_of(answers.begin(), answers.end(), [](auto id) {
        return id.id != -1;
    });
    pthread_mutex_unlock(&answer_access_mutex);
    return answer;
}

void get_next_answer_id(int& i) {
    pthread_mutex_lock(&answer_access_mutex);
    i %= answers.size();
    int k = i;
    do {
        if (answers[i].id == -1) {
            pthread_mutex_unlock(&answer_access_mutex);
            return;
        }
        i = (i + 1) % answers.size();
    }
    while (k != i);
    pthread_mutex_unlock(&answer_access_mutex);
}

int main(int argc, char** argv) {
    pthread_mutex_init(&client_access_mutex, nullptr);
    pthread_mutex_init(&answer_access_mutex, nullptr);
    pthread_mutex_init(&log_mutex, nullptr);

    if (argc != 4) {
        Die("ARG FORMAT: <SERVER IP, SERVER PORT,MESSAGE>");
    }
    auto message = std::string(argv[3]);
    std::vector<std::string> tasks = split(message);
    tasks[tasks.size() - 1] += std::string(TASK_SIZE - tasks[tasks.size() - 1].size(), ' ');
    for (int i = 0; i < tasks.size(); ++i) {
        answers.push_back({.id = -1});
    }

    const char* server_ip = argv[1];
    unsigned short port = atoi(argv[2]);
    clients = std::vector<sockaddr_in>();
    clients_sock = std::vector<int>();
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
#define  MAX_PENDING 10
    if (listen(serv_sock, MAX_PENDING) < 0) {
        Die("listen() failed");
    }
    else {
        std::cout << "listening\n";
    }

    pthread_t listener_accepter;
    pthread_create(&listener_accepter, nullptr, accept_all, (void*)&serv_sock);
    pthread_t answer_accepter;
    pthread_create(&answer_accepter, nullptr, get_answers, nullptr);
    pthread_t tmp;
    std::string msg;

    pthread_mutex_lock(&log_mutex);
    msg = "server are ready to send all tasks";
    tmp = send_log_async(msg);
    log_threads.push_back(tmp);
    std::cout << msg << '\n';
    pthread_mutex_unlock(&log_mutex);

    std::cout << "press enter to start\n";
    getchar();
    int i = 0;
    int j = 0;
    std::vector<int> bad_sockets{};
    while (!check_end()) {
        if (clients_sock.empty()) {
            pthread_cond_wait(&empty_lock, &client_access_mutex);
        }
        else {
            pthread_mutex_lock(&client_access_mutex);
        }
        get_next_answer_id(i);
        j = j % clients_sock.size();
        ssize_t size = send_task(i, clients_sock[j], tasks[i], MSG_NOSIGNAL);
        if (size == 0) {
            clients_sock.erase(clients_sock.begin() + j);
            clients.erase(clients.begin() + j);
            pthread_mutex_unlock(&client_access_mutex);
            continue;
        }
        pthread_mutex_lock(&log_mutex);
        msg = "task " + tasks[i] + " was sent to worker (sock: " + std::to_string(clients_sock[j]) + ")";
        std::cout << msg << '\n';
        tmp = send_log_async(msg);
        log_threads.push_back(tmp);
        pthread_mutex_unlock(&log_mutex);
        ++j;
        ++i;
        pthread_mutex_unlock(&client_access_mutex);
        msleep(30);
    }

    pthread_mutex_lock(&log_mutex);
    msg = "all data were gotten";
    pthread_cancel(answer_accepter);
    std::cout << msg << '\n';
    tmp = send_log_async(msg);
    log_threads.push_back(tmp);

    msg = "in order:";
    std::cout << msg << '\n';
    tmp = send_log_async(msg);
    log_threads.push_back(tmp);
    pthread_mutex_unlock(&log_mutex);
    for (int i = 0; i < tasks.size(); ++i) {
        pthread_mutex_lock(&log_mutex);
        msg = "id: " + std::to_string(i) + ", answer: " + answers[i].task;
        std::cout << msg << '\n';
        tmp = send_log_async(msg);
        log_threads.push_back(tmp);
        pthread_mutex_unlock(&log_mutex);
    }

    pthread_cancel(listener_accepter);
    pthread_mutex_lock(&log_mutex);
    msg = "exit";
    pthread_create(&tmp, nullptr, send_log, new std::string(msg));
    pthread_join(tmp, nullptr);
    pthread_mutex_lock(&client_access_mutex);
    pthread_mutex_unlock(&log_mutex);
    for (auto& item : clients_sock) {
        close(item);
    }
    for (auto& item : log_threads) {
        pthread_cancel(item);
    }
    for (auto& item : listeners_sock) {
        close(item);
    }
    pthread_mutex_destroy(&client_access_mutex);
    pthread_mutex_destroy(&answer_access_mutex);
}
