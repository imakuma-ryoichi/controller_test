#pragma once

#include <array>
#include <cstdint>

struct ControllerMapping
{
    std::array<uint8_t, 8> axis_events{};
    std::array<uint8_t, 13> button_events{};
    uint32_t send_rate_hz = 100;
};

bool load_controller_mapping(const char* path, ControllerMapping& mapping);
