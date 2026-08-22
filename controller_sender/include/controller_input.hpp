#pragma once

#include "controller_data.hpp"
#include "controller_id.hpp"

int open_controller(const char* device);
int open_touchpad_event(const char* device);
bool update_controller(
    int controller_fd,
    const ControllerMapping& mapping,
    ControllerData& data);
bool update_touchpad_event(
    int touchpad_fd,
    uint16_t touchpad_button_code,
    ControllerData& data);
