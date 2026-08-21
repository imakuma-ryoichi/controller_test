#pragma once

#include "controller_data.hpp"

#include <cstdint>

int connect_bluetooth(const char* address, uint8_t channel);
bool send_bluetooth(int socket_fd, const ControllerData& data);
