#include "bluetooth_sender.hpp"

#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>

int connect_bluetooth(const char* address, uint8_t channel)
{
    const int socket_fd = socket(
        AF_BLUETOOTH,
        SOCK_STREAM,
        BTPROTO_RFCOMM);

    if (socket_fd < 0) {
        std::cerr << "Bluetooth Socketの作成に失敗しました\n";
        return -1;
    }

    sockaddr_rc receiver{};
    receiver.rc_family = AF_BLUETOOTH;
    receiver.rc_channel = channel;

    if (str2ba(address, &receiver.rc_bdaddr) != 0) {
        std::cerr << "Bluetoothアドレスが不正です\n";
        close(socket_fd);
        return -1;
    }

    if (connect(
        socket_fd,
        reinterpret_cast<sockaddr*>(&receiver),
        sizeof(receiver)) < 0)
    {
        std::cerr << "Bluetooth接続に失敗しました: "
                  << std::strerror(errno) << '\n';
        close(socket_fd);
        return -1;
    }

    return socket_fd;
}

bool send_bluetooth(int socket_fd, const ControllerData& data)
{
    const ssize_t size = send(
        socket_fd,
        &data,
        sizeof(data),
        0);

    return size == sizeof(data);
}
