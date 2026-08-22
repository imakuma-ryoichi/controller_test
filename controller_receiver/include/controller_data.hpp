#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

constexpr std::size_t AXES_COUNT = 8;
constexpr std::size_t JS_BUTTON_COUNT = 13;
constexpr std::size_t TOUCHPAD_BUTTON_INDEX = JS_BUTTON_COUNT;
constexpr std::size_t BUTTON_COUNT = JS_BUTTON_COUNT + 1;

struct ControllerData
{
    std::array<int32_t, AXES_COUNT> axes{};
    std::array<int32_t, BUTTON_COUNT> buttons{};
};
