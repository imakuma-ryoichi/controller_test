#pragma once

#include <array>
#include <cstdint>
#include <string>

struct ControllerMapping
{
    std::array<uint8_t, 8> axis_events{};
    std::array<uint8_t, 13> button_events{};
    bool touchpad_enabled = true;
    std::string touchpad_event_device{};
    uint16_t touchpad_button_code{};
    uint32_t send_rate_hz = 100;
};

bool load_controller_mapping(const char* path, ControllerMapping& mapping);
