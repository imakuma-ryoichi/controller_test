#include "controller_input.hpp"
#include "wifi_sender.hpp"
#include "bluetooth_sender.hpp"

#include <unistd.h>
#include <chrono>
#include <iostream>
#include <thread>

int main()
{
    constexpr char DEVICE[] = "/dev/input/js0";
    constexpr auto PERIOD = std::chrono::milliseconds(10);

    const int controller_fd = open_controller(DEVICE);

    if (controller_fd < 0) {
        return 1;
    }

    ControllerData data{};

    WiFiSender wifi_sender{};
    BluetoothSender bluetooth_sender{};

    while (true) {
        update_controller(controller_fd, data);

        wifi_sender.send(data);
        bluetooth_sender.send(data);

        std::this_thread::sleep_for(PERIOD);
    }

    close(controller_fd);

    return 0;
}
