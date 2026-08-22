#pragma once

#include <array>
#include <cstdint>

struct ControllerData
{
    // Left/right stick X/Y and the analog L2/R2 values.
    std::array<int32_t, 6> axes{};
    // The selected digital inputs, represented as 0 or 1.
    std::array<int32_t, 7> buttons{};
};
