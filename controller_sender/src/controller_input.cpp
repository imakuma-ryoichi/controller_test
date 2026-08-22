#include "controller_input.hpp"

#include <fcntl.h>
#include <linux/joystick.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>

namespace
{
constexpr uint8_t X_AXIS = 0;
constexpr uint8_t Y_AXIS = 1;
constexpr uint8_t ROTATION_AXIS = 2;
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
            switch (event.number) {
            case X_AXIS:
                data.x = event.value;
                updated = true;
                break;
            case Y_AXIS:
                data.y = event.value;
                updated = true;
                break;
            case ROTATION_AXIS:
                data.rotation = event.value;
                updated = true;
                break;
            default:
                break;
            }
            break;

        default:
            break;
        }
    }

    return updated;
}
