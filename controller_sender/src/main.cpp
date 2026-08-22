#include <unistd.h>

#include <iostream>
#include <string>

#include "bluetooth_sender.hpp"
#include "controller_input.hpp"
#include "wifi_sender.hpp"

int main(int argc, char* argv[])
{
    const char* config_path = "config/controller_id.yaml";
    if (argc == 3 && std::string(argv[1]) == "--config") {
        config_path = argv[2];
    } else if (argc != 1) {
        std::cerr << "使い方: controller_sender [--config path]\n";
        return 1;
    }

    ControllerMapping mapping{};
    if (!load_controller_mapping(config_path, mapping)) {
        return 1;
    }

    constexpr const char* controller_device = "/dev/input/js0";
    constexpr const char* wifi_address = "192.168.1.100";
    constexpr uint16_t wifi_port = 5000;
    constexpr const char* bluetooth_address = "XX:XX:XX:XX:XX:XX";
    constexpr uint8_t bluetooth_channel = 1;

    const int controller_fd = open_controller(controller_device);

    if (controller_fd < 0) {
        return 1;
    }
    const int touchpad_fd = open_touchpad_event(
        mapping.touchpad_event_device.c_str(),
        mapping.touchpad_button_code);
    if (touchpad_fd < 0) {
        close(controller_fd);
        return 1;
    }

    const int wifi_fd = create_wifi_sender(wifi_address, wifi_port);

    if (wifi_fd < 0) {
        close(touchpad_fd);
        close(controller_fd);
        return 1;
    }

    const int bluetooth_fd =
        connect_bluetooth(bluetooth_address, bluetooth_channel);

    if (bluetooth_fd < 0) {
        close(wifi_fd);
        close(touchpad_fd);
        close(controller_fd);
        return 1;
    }

    ControllerData data{};

    while (true) {
        const bool joystick_updated = update_controller(controller_fd, mapping, data);
        const bool touchpad_updated = update_touchpad_event(
            touchpad_fd,
            mapping.touchpad_button_code,
            data);

        if (joystick_updated || touchpad_updated) {
            send_wifi(wifi_fd, data);
            send_bluetooth(bluetooth_fd, data);
        }

        usleep(1000);
    }

    close(bluetooth_fd);
    close(wifi_fd);
    close(touchpad_fd);
    close(controller_fd);

    return 0;
}
