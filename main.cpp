#include <fcntl.h>
#include <linux/input.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <thread>

struct ControllerData
{
    int32_t x{};
    int32_t y{};
    int32_t rotation{};
};

std::atomic<bool> running{true};

void signal_handler(int)
{
    running = false;
}

int main()
{
    constexpr char DEVICE[] = "/dev/input/event0";
    constexpr char RECEIVER_IP[] = "192.168.1.100";
    constexpr int PORT = 5000;
    constexpr auto PERIOD = std::chrono::milliseconds(10);

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    const int controller_fd = open(DEVICE, O_RDONLY | O_NONBLOCK);

    if (controller_fd < 0) {
        std::cerr << "コントローラーを開けません: "
                  << std::strerror(errno) << '\n';
        return 1;
    }

    const int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);

    if (socket_fd < 0) {
        std::cerr << "Socketの作成に失敗しました: "
                  << std::strerror(errno) << '\n';
        close(controller_fd);
        return 1;
    }

    sockaddr_in receiver{};
    receiver.sin_family = AF_INET;
    receiver.sin_port = htons(PORT);

    if (inet_pton(AF_INET, RECEIVER_IP, &receiver.sin_addr) != 1) {
        std::cerr << "受信側IPアドレスが不正です\n";
        close(socket_fd);
        close(controller_fd);
        return 1;
    }

    ControllerData data{};
    auto next_send = std::chrono::steady_clock::now();

    while (running) {
        input_event event{};

        while (read(controller_fd, &event, sizeof(event)) == sizeof(event)) {
            if (event.type != EV_ABS) {
                continue;
            }

            switch (event.code) {
            case ABS_X:
                data.x = event.value;
                break;

            case ABS_Y:
                data.y = event.value;
                break;

            case ABS_RX:
                data.rotation = event.value;
                break;

            default:
                break;
            }
        }

        const ssize_t size = sendto(
            socket_fd,
            &data,
            sizeof(data),
            0,
            reinterpret_cast<sockaddr*>(&receiver),
            sizeof(receiver));

        if (size != sizeof(data)) {
            std::cerr << "UDP送信に失敗しました: "
                      << std::strerror(errno) << '\n';
        }

        next_send += PERIOD;
        std::this_thread::sleep_until(next_send);
    }

    close(socket_fd);
    close(controller_fd);

    std::cout << "コントローラー送信を終了しました\n";

    return 0;
}
