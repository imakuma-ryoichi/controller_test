#include "bluetooth_sender.hpp"
#include "controller_input.hpp"
#include "wifi_sender.hpp"

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <iostream>
#include <thread>
#include <unistd.h>

namespace
{
std::atomic<bool> running{true};

void signal_handler(int)
{
    running = false;
}
}

int main()
{
    constexpr char DEVICE[] = "/dev/input/event12";
    constexpr char RECEIVER_IP[] = "192.168.1.100";
    constexpr uint16_t WIFI_PORT = 5000;

    constexpr char BLUETOOTH_ADDRESS[] = "00:00:00:00:00:00";
    constexpr uint8_t BLUETOOTH_CHANNEL = 1;

    constexpr auto PERIOD = std::chrono::milliseconds(10);

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    const int controller_fd = open_controller(DEVICE);

    if (controller_fd < 0) {
        return 1;
    }

    const int wifi_fd = create_wifi_sender(
        RECEIVER_IP,
        WIFI_PORT);

    if (wifi_fd < 0) {
        close(controller_fd);
        return 1;
    }

    const int bluetooth_fd = connect_bluetooth(
        BLUETOOTH_ADDRESS,
        BLUETOOTH_CHANNEL);

    ControllerData data{};

    auto next_send = std::chrono::steady_clock::now();

    while (running) {
        update_controller(controller_fd, data);

        if (!send_wifi(wifi_fd, data)) {
            std::cerr << "Wi-Fi送信に失敗しました\n";
        }

        if (bluetooth_fd >= 0) {
            if (!send_bluetooth(bluetooth_fd, data)) {
                std::cerr << "Bluetooth送信に失敗しました\n";
            }
        }

        next_send += PERIOD;
        std::this_thread::sleep_until(next_send);
    }

    close(wifi_fd);

    if (bluetooth_fd >= 0) {
        close(bluetooth_fd);
    }

    close(controller_fd);

    return 0;
}
