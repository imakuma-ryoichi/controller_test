#include "controller_input.hpp"

#include <fcntl.h>
#include <linux/joystick.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>

namespace
{
enum class Axis : uint8_t
{
    LeftX = 0,
    LeftY = 1,
    RightX = 2,
    RightY = 3,
    LeftTrigger = 4,
    RightTrigger = 5,
    DPadX = 6,
    DPadY = 7
};
}

int open_controller(const char* device)
{
    const int fd = open(device, O_RDONLY | O_NONBLOCK);

    if (fd < 0) {
        std::cerr << "コントローラーを開けません: "
                  << std::strerror(errno) << '\n';
    }

    return fd;
}

bool update_controller(int controller_fd, ControllerData& data)
{
    bool updated = false;
    js_event event{};

    while (read(controller_fd, &event, sizeof(event)) == sizeof(event)) {
        event.type &= ~JS_EVENT_INIT;

        switch (event.type) {
        case JS_EVENT_AXIS:
            if (event.number < data.axes.size()) {
                data.axes[event.number] = event.value;
                updated = true;
            }
            break;

        case JS_EVENT_BUTTON:
            if (event.number < data.buttons.size()) {
                data.buttons[event.number] = event.value;
                updated = true;
            }
            break;

        default:
            break;
        }
    }

    return updated;
}
