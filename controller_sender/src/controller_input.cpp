#include "controller_input.hpp"

#include <fcntl.h>
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
