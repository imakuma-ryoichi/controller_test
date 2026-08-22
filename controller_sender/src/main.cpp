#include <unistd.h>

#include <iostream>
#include <string>

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

    const int wifi_fd = create_wifi_sender(config.wifi_address.c_str(), config.wifi_port);

    if (wifi_fd < 0) {
        close(controller_fd);
        return 1;
    }

    const int bluetooth_fd =
        connect_bluetooth(config.bluetooth_address.c_str(), config.bluetooth_channel);

    if (bluetooth_fd < 0) {
        close(wifi_fd);
        close(controller_fd);
        return 1;
    }

    ControllerData data{};

    while (true) {
        if (update_controller(controller_fd, mapping, data)) {
            send_wifi(wifi_fd, data);
            send_bluetooth(bluetooth_fd, data);
        }

        usleep(1000);
    }

    close(bluetooth_fd);
    close(wifi_fd);
    close(controller_fd);

    return 0;
}
