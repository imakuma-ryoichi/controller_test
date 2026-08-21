#include "controller_input.hpp"

#include <fcntl.h>
#include <linux/input.h>
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

bool update_controller(int controller_fd, ControllerData& data)
{
    bool updated = false;
    input_event event{};

    while (read(controller_fd, &event, sizeof(event)) == sizeof(event)) {
        if (event.type != EV_ABS) {
            continue;
        }

        switch (event.code) {
        case ABS_X:
            data.x = event.value;
            updated = true;
            break;

        case ABS_Y:
            data.y = event.value;
            updated = true;
            break;

        case ABS_RX:
            data.rotation = event.value;
            updated = true;
            break;

        default:
            break;
        }
    }

    return updated;
}
