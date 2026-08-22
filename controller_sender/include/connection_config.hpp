#pragma once

#include <cstdint>
#include <string>

struct ConnectionConfig
{
    std::string controller_device = "/dev/input/js0";
    bool wifi_enabled = true;
    std::string wifi_address = "192.168.1.100";
    uint16_t wifi_port = 5000;
    bool bluetooth_enabled = true;
    std::string bluetooth_address = "XX:XX:XX:XX:XX:XX";
    uint8_t bluetooth_channel = 1;
};

bool load_connection_config(const char* path, ConnectionConfig& config);
