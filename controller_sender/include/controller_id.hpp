#pragma once

#include <array>
#include <cstdint>
#include <string>

struct ControllerMapping
{
    std::array<uint8_t, 8> axis_events{};
    std::array<uint8_t, 13> button_events{};
    std::string touchpad_event_device{};
    uint16_t touchpad_button_code{};
};

bool load_controller_mapping(const char* path, ControllerMapping& mapping);
