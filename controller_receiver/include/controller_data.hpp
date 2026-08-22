#pragma once

#include <array>
#include <cstdint>

struct ControllerData
{
    std::array<int32_t, 6> axes{};
    std::array<int32_t, 7> buttons{};
};
