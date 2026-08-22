#include <unistd.h>

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#include "bluetooth_sender.hpp"
#include "connection_config.hpp"
#include "controller_input.hpp"
#include "wifi_sender.hpp"

int main(int argc, char* argv[])
{
    const char* mapping_config_path = "config/controller_id.yaml";
    const char* connection_config_path = "config/controller_connection.yaml";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--config" && i + 1 < argc) {
            mapping_config_path = argv[++i];
        } else if (arg == "--connection-config" && i + 1 < argc) {
            connection_config_path = argv[++i];
        } else {
            std::cerr << "使い方: controller_sender [--config mapping_path] [--connection-config connection_path]\n";
            return 1;
        }
    }

    ControllerMapping mapping{};

    if (!load_controller_mapping(mapping_config_path, mapping)) {
        return 1;
    }

    ConnectionConfig config{};

    if (!load_connection_config(connection_config_path, config)) {
        return 1;
    }

    const int controller_fd = open_controller(config.controller_device.c_str());

    if (controller_fd < 0) {
        return 1;
    }

    int touchpad_fd = -1;
    if (mapping.touchpad_enabled) {
        touchpad_fd = open_touchpad_event(
            mapping.touchpad_event_device.c_str(),
            mapping.touchpad_button_code);

        if (touchpad_fd < 0) {
            close(controller_fd);
            return 1;
        }
    }

    int wifi_fd = -1;
    if (config.wifi_enabled) {
        wifi_fd =
            create_wifi_sender(config.wifi_address.c_str(), config.wifi_port);

        if (wifi_fd < 0) {
            if (touchpad_fd >= 0) {
                close(touchpad_fd);
            }
            close(controller_fd);
            return 1;
        }
    }

    if (!config.wifi_enabled && !config.bluetooth_enabled) {
        std::cerr << "Wi-FiとBluetoothのどちらかは有効にしてください\n";
        if (touchpad_fd >= 0) {
            close(touchpad_fd);
        }
        close(controller_fd);
        return 1;
    }

    int bluetooth_fd = -1;

    ControllerData data{};

    const auto period =
        std::chrono::nanoseconds(1'000'000'000 / mapping.send_rate_hz);
    auto next_tick = std::chrono::steady_clock::now();
    auto next_bluetooth_retry = next_tick;

    while (true) {
        const auto now = std::chrono::steady_clock::now();

        if (config.bluetooth_enabled &&
            bluetooth_fd < 0 &&
            now >= next_bluetooth_retry)
        {
            bluetooth_fd =
                connect_bluetooth(
                    config.bluetooth_address.c_str(),
                    config.bluetooth_channel);

            next_bluetooth_retry = now + std::chrono::seconds(1);
        }

        update_controller(controller_fd, mapping, data);
        if (touchpad_fd >= 0) {
            update_touchpad_event(
                touchpad_fd,
                mapping.touchpad_button_code,
                data);
        }

        if (wifi_fd >= 0 && !send_wifi(wifi_fd, data)) {
            std::cerr << "Wi-Fiデータの送信に失敗しました\n";
        }

        if (bluetooth_fd >= 0 && !send_bluetooth(bluetooth_fd, data)) {
            std::cerr << "Bluetooth切断を検出しました。再接続します\n";
            close(bluetooth_fd);
            bluetooth_fd = -1;
            next_bluetooth_retry = now;
        }

        next_tick += period;
        const auto after_work = std::chrono::steady_clock::now();
        if (next_tick < after_work) {
            next_tick = after_work + period;
        }
        std::this_thread::sleep_until(next_tick);
    }

    if (bluetooth_fd >= 0) {
        close(bluetooth_fd);
    }
    if (wifi_fd >= 0) {
        close(wifi_fd);
    }
    if (touchpad_fd >= 0) {
        close(touchpad_fd);
    }
    close(controller_fd);

    return 0;
}
