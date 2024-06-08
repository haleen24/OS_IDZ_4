#include "lib.h"

void init_default_dict() {
    for (unsigned char i = 0; i < 128; ++i) {
        dict[static_cast<char>(i)] = static_cast<char>((i + SHIFT) % 128);
    }
}

void init_dict(const std::unordered_map<char, char>& new_dict) {
    if (new_dict.empty()) {
        return init_default_dict();
    }
    dict = new_dict;
}

void Die(const std::string& message) {
    std::cout << message << '\n';
    exit(-1);
}


std::vector<std::string> split(const std::string& str) {
    std::vector<std::string> res(str.size() / TASK_SIZE + (str.size() % TASK_SIZE != 0));
    auto iter = res.begin();
    for (int i = 0; i < str.size(); i += TASK_SIZE) {
        *iter = std::string(str, i, TASK_SIZE);
        ++iter;
    }
    return res;
}

std::string convert_id_to_message(int32_t id) {
    std::string id_str = std::to_string(id);
    std::string str(ID_SIZE - id_str.size(), '0');
    return str + id_str;
}

ssize_t send_task(int32_t id, int socket, const std::string task, int flag, sockaddr* sockaddr, socklen_t addr_len) {
    ssize_t size = sendto(socket, (convert_id_to_message(id) + task).c_str(), ID_SIZE + task.size(), flag, sockaddr,
                          addr_len);
    if (size <= 0) {
        return 0;
    }
    return 1;
}

TaskStruct recive_task(int socket, int flag, sockaddr* sockaddr, socklen_t* t) {
    char bigbuff[ID_SIZE + TASK_SIZE];
    ssize_t size = recvfrom(socket, bigbuff, ID_SIZE + TASK_SIZE, flag, sockaddr, t);
    if (size < 0) {
        Die("recvfrom() failed");
    }
    else if (size == 0) {
        return TaskStruct{
            .id = -1,
            .socket = -1,
            .task = "",
            .flag = 0
        };
    }
    int id_val = std::stoi(std::string(bigbuff, ID_SIZE));
    return TaskStruct{
        .id = id_val,
        .socket = socket,
        .task = std::string(bigbuff, ID_SIZE, size - ID_SIZE),
        .flag = 0
    };
}


std::string encode(const std::string& s) {
    std::string res;
    for (auto& item : s) {
        res += dict[item];
    }
    return res;
}

void sig_handler(const int sig) {
    if (sig == SIGINT)
        exit(0);
}

void set_signal_handler() {
    struct sigaction handler;
    handler.sa_handler = &sig_handler;
    if (sigfillset(&handler.sa_mask) < 0) {
        Die("sigfillset() failed");
    }
    if (sigaction(SIGINT, &handler, 0) < 0) {
        Die("sigaction() failed");
    }
}

std::string recive_log(int socket, int flag = 0) {
    char buff[LOG_SIZE];
    ssize_t size = recvfrom(socket, buff, LOG_SIZE, flag, NULL, 0);
    if (size == 0) {
        std::cout << "Server reach the end, exit...\n";
        exit(0);
    }
    if (size < 0) {
        Die("recv() failed");
    }
    return {buff};
}

int msleep(long msec) {
    struct timespec ts;
    int res;

    if (msec < 0) {
        errno = EINVAL;
        return -1;
    }

    ts.tv_sec = msec / 1000;
    ts.tv_nsec = (msec % 1000) * 1000000;

    do {
        res = nanosleep(&ts, &ts);
    }
    while (res && errno == EINTR);

    return res;
}
