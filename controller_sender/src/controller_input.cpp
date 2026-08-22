#include "controller_input.hpp"

#include <fcntl.h>
#include <linux/joystick.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>

namespace
{
constexpr uint8_t AXIS_COUNT = 6;
constexpr uint8_t BUTTON_COUNT = 7;
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
            if (event.number < AXIS_COUNT) {
                data.axes[event.number] = event.value;
                updated = true;
            }
            break;

        case JS_EVENT_BUTTON:
            if (event.number < BUTTON_COUNT) {
                data.buttons[event.number] = event.value != 0 ? 1 : 0;
                updated = true;
            }
            break;

        default:
            break;
        }
    }

    return updated;
}
