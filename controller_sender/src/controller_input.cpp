#include "controller_input.hpp"

#include <fcntl.h>
#include <linux/input-event-codes.h>
#include <linux/input.h>
#include <linux/joystick.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>
#include <string>

int open_controller(const char* device)
{
    const int fd = open(device, O_RDONLY | O_NONBLOCK);

    if (fd < 0) {
        std::cerr << "コントローラーを開けません: "
                  << std::strerror(errno) << '\n';
    }

    return fd;
}

namespace
{
bool supports_code(int fd, unsigned int event_type, unsigned int code)
{
    unsigned long bits[KEY_MAX / (sizeof(unsigned long) * 8) + 1]{};
    const int bytes = ioctl(
        fd,
        EVIOCGBIT(event_type, sizeof(bits)),
        bits);

    if (bytes < 0) {
        return false;
    }

    const std::size_t word = code / (sizeof(unsigned long) * 8);
    const std::size_t bit = code % (sizeof(unsigned long) * 8);
    return word < sizeof(bits) / sizeof(bits[0])
        && (bits[word] & (1UL << bit)) != 0;
}

bool supports_touchpad(int fd, uint16_t button_code)
{
    return supports_code(fd, EV_KEY, button_code)
        && supports_code(fd, EV_ABS, ABS_MT_POSITION_X);
}
}

int open_touchpad_event(const char* device, uint16_t touchpad_button_code)
{
    const int fd = open(device, O_RDONLY | O_NONBLOCK);

    if (fd >= 0 && supports_touchpad(fd, touchpad_button_code)) {
        return fd;
    }

    if (fd >= 0) {
        close(fd);
        std::cerr << "指定されたデバイスにタッチパッドボタンがありません: "
                  << device << '\n';
    } else {
        std::cerr << "指定されたタッチパッド入力デバイスを開けません: "
                  << device << ": " << std::strerror(errno) << '\n';
    }

    for (unsigned int index = 0; index < 32; ++index) {
        const std::string candidate = "/dev/input/event" + std::to_string(index);
        const int candidate_fd = open(candidate.c_str(), O_RDONLY | O_NONBLOCK);
        if (candidate_fd < 0) {
            continue;
        }
        if (supports_touchpad(candidate_fd, touchpad_button_code)) {
            std::cerr << "タッチパッド入力デバイスを自動選択: "
                      << candidate << '\n';
            return candidate_fd;
        }
        close(candidate_fd);
    }

    std::cerr << "タッチパッドボタンを持つ入力デバイスが見つかりません\n";
    return -1;
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
