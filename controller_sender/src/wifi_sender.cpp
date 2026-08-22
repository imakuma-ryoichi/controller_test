#include "wifi_sender.hpp"

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>

int create_wifi_sender(const char* ip, uint16_t port)
{
    const int socket_fd = socket(AF_INET, SOCK_STREAM, 0);

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

    return socket_fd;
}

bool send_wifi(int socket_fd, const ControllerData& data)
{
    const auto* bytes = reinterpret_cast<const char*>(&data);
    std::size_t sent = 0;

    while (sent < sizeof(data)) {
        const ssize_t size = send(
            socket_fd,
            bytes + sent,
            sizeof(data) - sent,
            0);

        if (size <= 0) {
            return false;
        }

        sent += static_cast<std::size_t>(size);
    }

    return true;
}
