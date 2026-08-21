#include "wifi_receiver.hpp"
#include "controller_data.hpp"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdint>
#include <iostream>

void receive_wifi()
{
    constexpr uint16_t PORT = 5000;

    const int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);

    if (socket_fd < 0) {
        std::cerr << "Wi-Fi Socketの作成に失敗しました\n";
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

        if (size != sizeof(data)) {
            continue;
        }

        std::cout
            << "[Wi-Fi] "
            << "x=" << data.x
            << " y=" << data.y
            << " rotation=" << data.rotation
            << '\n';
    }
}
