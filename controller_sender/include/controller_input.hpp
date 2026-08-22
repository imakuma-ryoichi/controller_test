#pragma once

#include "controller_data.hpp"
#include "controller_id.hpp"

int open_controller(const char* device);
bool update_controller(
    int controller_fd,
    const ControllerMapping& mapping,
    ControllerData& data);
