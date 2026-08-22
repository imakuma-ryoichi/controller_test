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

    sockaddr_in receiver_addr{};
    receiver_addr.sin_family = AF_INET;
    receiver_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &receiver_addr.sin_addr) != 1) {
        std::cerr << "受信側IPアドレスが不正です\n";
        close(socket_fd);
        return -1;
    }

    if (connect(
        socket_fd,
        reinterpret_cast<sockaddr*>(&receiver_addr),
        sizeof(receiver_addr)) < 0)
    {
        std::cerr << "Wi-Fi接続に失敗しました: "
                  << std::strerror(errno) << '\n';
        close(socket_fd);
        return -1;
    }

    std::cout << "Wi-Fi送信先: UDP/" << ip << ':' << port << '\n';
    return socket_fd;
}

bool send_wifi(int socket_fd, const ControllerData& data)
{
    const auto* bytes = reinterpret_cast<const char*>(&data);
    const ssize_t size = send(socket_fd, bytes, sizeof(data), MSG_NOSIGNAL);
    return size == static_cast<ssize_t>(sizeof(data));
}
