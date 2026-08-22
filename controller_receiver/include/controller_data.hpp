#pragma once

#include <array>
#include <cstdint>

struct ControllerData
{
    std::array<int32_t, 8> axes{};
    std::array<int32_t, 13> buttons{};
};
