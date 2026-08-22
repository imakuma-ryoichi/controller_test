#pragma once

#include <array>
#include <cstdint>

struct ControllerData
{
    std::array<int16_t, 8> axes{};
    std::array<uint8_t, 16> buttons{};
};
