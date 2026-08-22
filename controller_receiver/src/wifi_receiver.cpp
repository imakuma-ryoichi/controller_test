#include "wifi_receiver.hpp"
#include "controller_data.hpp"

#include <arpa/inet.h>
#include <cerrno>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdint>
#include <iostream>
#include <string>

void receive_wifi()
{
    constexpr uint16_t PORT = 5000;

    const int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);

    if (socket_fd < 0) {
        std::cerr << "Wi-Fi Socketの作成に失敗しました\n";
        return;
    }

    const int reuse = 1;
    if (setsockopt(
        socket_fd,
        SOL_SOCKET,
        SO_REUSEADDR,
        &reuse,
        sizeof(reuse)) < 0)
    {
        std::cerr << "Wi-Fi Socketの設定に失敗しました\n";
        close(socket_fd);
        return;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(
        socket_fd,
        reinterpret_cast<sockaddr*>(&address),
        sizeof(address)) < 0)
    {
        std::cerr << "Wi-Fi Socketのbindに失敗しました\n";
        close(socket_fd);
        return;
    }

    std::cout << "Wi-Fi受信待機中: UDP/" << PORT << '\n';

    while (true) {
        ControllerData data{};

        const ssize_t size = recvfrom(
            socket_fd,
            &data,
            sizeof(data),
            0,
            nullptr,
            nullptr);

        if (size < 0) {
            if (errno == EINTR) {
                continue;
            }
            break;
        }

        if (size != static_cast<ssize_t>(sizeof(data))) {
            continue;
        }

        std::string output = "[Wi-Fi] axes=";
        for (const int32_t value : data.axes) {
            output += std::to_string(value) + ' ';
        }
        output += "buttons=";
        for (const int32_t value : data.buttons) {
            output += std::to_string(value) + ' ';
        }
        output += '\n';
        std::cout << output << std::flush;
    }

    close(socket_fd);
}
