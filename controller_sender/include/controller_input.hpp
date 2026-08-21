#pragma once

#include "controller_data.hpp"

int open_controller(const char* device);
bool update_controller(int controller_fd, ControllerData& data);
