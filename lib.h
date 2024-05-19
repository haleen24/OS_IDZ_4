#pragma once

#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <signal.h>
#include <unordered_map>
#include <vector>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>
#define TASK_SIZE 10ul
// #define ANSWER_SIZE (3*TASK_SIZE)
#define ID_SIZE 10ul
#define SHIFT 1
#define LOG_SIZE 255
inline std::unordered_map<char, char> dict{};

void init_default_dict();

void init_dict(const std::unordered_map<char, char>& new_dict = {});

void Die(const std::string& message);

std::vector<std::string> split(const std::string& str);

ssize_t send_task(int32_t id, int socket, std::string task, int flag = 0);

struct TaskStruct {
    int32_t id;
    int socket;
    std::string task;
    int flag = 0;
};

inline void* send_task(void* send) {
    const auto tmp = static_cast<TaskStruct*>(send);
    send_task(tmp->id, tmp->socket, tmp->task, tmp->flag);
    return nullptr;
}

TaskStruct recive_task(int socket, int flag = 0);

std::string encode(const std::string& s);

void sig_handler(int);

void set_signal_handler();

std::string recive_log(int);

int msleep(long msec);
