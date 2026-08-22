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

    const int socket_fd = socket(AF_INET, SOCK_STREAM, 0);

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

    if (listen(socket_fd, 1) < 0) {
        std::cerr << "Wi-Fiのlistenに失敗しました\n";
        close(socket_fd);
        return;
    }

    std::cout << "Wi-Fi受信待機中: TCP/" << PORT << '\n';

    while (true) {
        const int client_fd = accept(socket_fd, nullptr, nullptr);

        if (client_fd < 0) {
            continue;
        }

        std::cout << "Wi-Fi接続\n";

        while (true) {
            ControllerData data{};

            const ssize_t size = recv(
                client_fd,
                &data,
                sizeof(data),
                MSG_WAITALL);

            if (size != sizeof(data)) {
                break;
            }

            std::cout << "[Wi-Fi] axes=";
            for (const int32_t value : data.axes) {
                std::cout << value << ' ';
            }
            std::cout << "buttons=";
            for (const int32_t value : data.buttons) {
                std::cout << value << ' ';
            }
            std::cout << '\n';
        }

        close(client_fd);
        std::cout << "Wi-Fi切断\n";
    }
}
