#include "wifi_sender.hpp"

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>

namespace
{
sockaddr_in g_receiver{};
}

int create_wifi_sender(const char* ip, uint16_t port)
{
    const int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);

    if (socket_fd < 0) {
        std::cerr << "Wi-Fi Socketの作成に失敗しました\n";
        return -1;
    }

    g_receiver = {};
    g_receiver.sin_family = AF_INET;
    g_receiver.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &g_receiver.sin_addr) != 1) {
        std::cerr << "受信側IPアドレスが不正です\n";
        close(socket_fd);
        return -1;
    }

    return socket_fd;
}

bool send_wifi(int socket_fd, const ControllerData& data)
{
    const ssize_t size = sendto(
        socket_fd,
        &data,
        sizeof(data),
        0,
        reinterpret_cast<const sockaddr*>(&g_receiver),
        sizeof(g_receiver));

    return size == static_cast<ssize_t>(sizeof(data));
}
