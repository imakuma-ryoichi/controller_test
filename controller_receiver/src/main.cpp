#include "bluetooth_receiver.hpp"
#include "receiver_connection_config.hpp"
#include "wifi_receiver.hpp"

#include <iostream>
#include <string>
#include <thread>

int main(int argc, char* argv[])
{
    const char* config_path = "config/receiver_connection.yaml";
    if (argc == 3 && std::string(argv[1]) == "--config") {
        config_path = argv[2];
    } else if (argc != 1) {
        std::cerr << "使い方: controller_receiver [--config path]\n";
        return 1;
    }

    ReceiverConnectionConfig config{};
    if (!load_receiver_connection_config(config_path, config)) {
        return 1;
    }

    std::thread wifi_thread(receive_wifi, config.wifi_port);
    std::thread bluetooth_thread(receive_bluetooth, config.bluetooth_channel);

    wifi_thread.join();
    bluetooth_thread.join();

    return 0;
}

