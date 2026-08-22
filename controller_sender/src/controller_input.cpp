#include "controller_input.hpp"

#include <fcntl.h>
#include <linux/input.h>
#include <linux/joystick.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>

int open_controller(const char* device)
{
    const int fd = open(device, O_RDONLY | O_NONBLOCK);

    if (fd < 0) {
        std::cerr << "コントローラーを開けません: "
                  << std::strerror(errno) << '\n';
    }

    return fd;
}

int open_touchpad_event(const char* device)
{
    const int fd = open(device, O_RDONLY | O_NONBLOCK);

    if (fd < 0) {
        std::cerr << "タッチパッド入力デバイスを開けません: "
                  << std::strerror(errno) << '\n';
    }

    return fd;
}

bool update_controller(
    int controller_fd,
    const ControllerMapping& mapping,
    ControllerData& data)
{
    bool updated = false;
    js_event event{};

    while (read(controller_fd, &event, sizeof(event)) == sizeof(event)) {
        event.type &= ~JS_EVENT_INIT;

        switch (event.type) {
        case JS_EVENT_AXIS:
            for (size_t index = 0; index < mapping.axis_events.size(); ++index) {
                if (event.number == mapping.axis_events[index]) {
                    data.axes[index] = event.value;
                    updated = true;
                    break;
                }
            }
            break;

        case JS_EVENT_BUTTON:
            for (size_t index = 0; index < mapping.button_events.size(); ++index) {
                if (event.number == mapping.button_events[index]) {
                    data.buttons[index] = event.value != 0 ? 1 : 0;
                    updated = true;
                    break;
                }
            }
            break;

        default:
            break;
        }
    }

    return updated;
}

bool update_touchpad_event(
    int touchpad_fd,
    uint16_t touchpad_button_code,
    ControllerData& data)
{
    bool updated = false;
    input_event event{};

    while (read(touchpad_fd, &event, sizeof(event)) == sizeof(event)) {
        if (event.type == EV_KEY && event.code == touchpad_button_code) {
            data.buttons[TOUCHPAD_BUTTON_INDEX] = event.value != 0 ? 1 : 0;
            updated = true;
        }
    }

    return updated;
}
