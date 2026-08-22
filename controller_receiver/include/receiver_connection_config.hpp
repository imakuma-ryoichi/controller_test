#pragma once

#include <cstdint>

struct ReceiverConnectionConfig
{
    uint16_t wifi_port = 5000;
    uint8_t bluetooth_channel = 1;
};

bool load_receiver_connection_config(const char* path, ReceiverConnectionConfig& config);
