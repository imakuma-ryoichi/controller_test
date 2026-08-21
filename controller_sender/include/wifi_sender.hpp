#pragma once

#include "controller_data.hpp"

#include <cstdint>

int create_wifi_sender(const char* ip, uint16_t port);
bool send_wifi(int socket_fd, const ControllerData& data);
