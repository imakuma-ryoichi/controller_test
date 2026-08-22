#include "bluetooth_receiver.hpp"
#include "controller_data.hpp"

#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>

void receive_bluetooth(uint8_t channel)
{
    const int socket_fd = socket(
        AF_BLUETOOTH,
        SOCK_STREAM,
        BTPROTO_RFCOMM);

    if (socket_fd < 0) {
        std::cerr << "Bluetooth Socketの作成に失敗しました\n";
        return;
    }

    sockaddr_rc address{};
    address.rc_family = AF_BLUETOOTH;
    address.rc_channel = channel;

    if (bind(
        socket_fd,
        reinterpret_cast<sockaddr*>(&address),
        sizeof(address)) < 0)
    {
        std::cerr << "Bluetooth Socketのbindに失敗しました\n";
        close(socket_fd);
        return;
    }

    if (listen(socket_fd, 1) < 0) {
        std::cerr << "Bluetoothのlistenに失敗しました\n";
        close(socket_fd);
        return;
    }

    std::cout
        << "Bluetooth受信待機中: RFCOMM/"
        << static_cast<int>(channel)
        << '\n';

    while (true) {
        sockaddr_rc client{};
        socklen_t client_size = sizeof(client);

        const int client_fd = accept(
            socket_fd,
            reinterpret_cast<sockaddr*>(&client),
            &client_size);

        if (client_fd < 0) {
            continue;
        }

        std::cout << "Bluetooth接続\n";

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

            std::cout << "[Bluetooth] axes=";
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

        std::cout << "Bluetooth切断\n";
    }
}
