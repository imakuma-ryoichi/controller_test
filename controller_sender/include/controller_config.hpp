#pragma once

#include <cstdint>

namespace controller_config
{
constexpr uint8_t AXIS_LEFT_STICK_X = 0;
constexpr uint8_t AXIS_LEFT_STICK_Y = 1;
constexpr uint8_t AXIS_L2 = 2;
constexpr uint8_t AXIS_RIGHT_STICK_X = 3;
constexpr uint8_t AXIS_RIGHT_STICK_Y = 4;
constexpr uint8_t AXIS_R2 = 5;
constexpr uint8_t AXIS_DPAD_LEFT_RIGHT = 6;
constexpr uint8_t AXIS_DPAD_UP_DOWN = 7;
constexpr uint8_t AXIS_COUNT = 8;

constexpr bool BUTTON_OFF = false;
constexpr bool BUTTON_ON = true;

constexpr uint8_t BUTTON_CROSS = 0;
constexpr uint8_t BUTTON_CIRCLE = 1;
constexpr uint8_t BUTTON_TRIANGLE = 2;
constexpr uint8_t BUTTON_SQUARE = 3;
constexpr uint8_t BUTTON_L1 = 4;
constexpr uint8_t BUTTON_R1 = 5;
constexpr uint8_t BUTTON_L2 = 6;
constexpr uint8_t BUTTON_R2 = 7;
constexpr uint8_t BUTTON_CREATE = 8;
constexpr uint8_t BUTTON_OPTION = 9;
constexpr uint8_t BUTTON_PS = 10;
constexpr uint8_t BUTTON_LEFT_STICK = 11;
constexpr uint8_t BUTTON_RIGHT_STICK = 12;
constexpr uint8_t BUTTON_COUNT = 13;
} // namespace controller_config
