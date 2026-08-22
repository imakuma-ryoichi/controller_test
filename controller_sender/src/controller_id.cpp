#include "controller_id.hpp"

#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>

namespace
{
const std::array<const char*, 8> AXIS_NAMES = {
    "left_stick_x", "left_stick_y", "l2", "right_stick_x",
    "right_stick_y", "r2", "dpad_left_right", "dpad_up_down"};

const std::array<const char*, 13> BUTTON_NAMES = {
    "cross", "circle", "triangle", "square", "l1", "r1", "l2",
    "r2", "create", "option", "ps", "left_stick", "right_stick"};

std::string trim(const std::string& value)
{
    const auto first = value.find_first_not_of(" \t\r");
    if (first == std::string::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(" \t\r");
    return value.substr(first, last - first + 1);
}

} // namespace

bool load_controller_mapping(const char* path, ControllerMapping& mapping)
{
    std::ifstream file(path);
    if (!file) {
        std::cerr << "コントローラー設定を開けません: " << path << '\n';
        return false;
    }

    std::map<std::string, std::string> values;
    std::string section;
    std::string line;
    while (std::getline(file, line)) {
        const auto comment = line.find('#');
        if (comment != std::string::npos) {
            line.erase(comment);
        }
        line = trim(line);
        if (line.empty()) {
            continue;
        }

        const auto separator = line.find(':');
        if (separator == std::string::npos) {
            std::cerr << "controller_id.yamlの形式が不正です: " << line << '\n';
            return false;
        }

        const std::string key = trim(line.substr(0, separator));
        const std::string value = trim(line.substr(separator + 1));
        if (value.empty()) {
            section = key;
            continue;
        }
        if (section != "axis" && section != "button" && section != "touchpad") {
            std::cerr << "controller_id.yamlのセクションが不正です: "
                      << section << '\n';
            return false;
        }
        values[section + "." + key] = value;
    }

    const auto parse_number = [&values](const std::string& full_key, unsigned long max, unsigned long& number) {
        const auto it = values.find(full_key);
        if (it == values.end()) {
            return false;
        }
        try {
            size_t parsed = 0;
            number = std::stoul(it->second, &parsed);
            if (parsed != it->second.size() || number > max) {
                throw std::out_of_range("event number");
            }
        } catch (const std::exception&) {
            return false;
        }
        return true;
    };

    for (size_t index = 0; index < AXIS_NAMES.size(); ++index) {
        const std::string key = "axis." + std::string(AXIS_NAMES[index]);
        unsigned long number = 0;
        if (!parse_number(key, std::numeric_limits<uint8_t>::max(), number)) {
            std::cerr << "controller_id.yamlにaxis." << AXIS_NAMES[index]
                      << "がないか、値が不正です\n";
            return false;
        }
        mapping.axis_events[index] = static_cast<uint8_t>(number);
    }
    for (size_t index = 0; index < BUTTON_NAMES.size(); ++index) {
        const std::string key = "button." + std::string(BUTTON_NAMES[index]);
        unsigned long number = 0;
        if (!parse_number(key, std::numeric_limits<uint8_t>::max(), number)) {
            std::cerr << "controller_id.yamlにbutton." << BUTTON_NAMES[index]
                      << "がないか、値が不正です\n";
            return false;
        }
        mapping.button_events[index] = static_cast<uint8_t>(number);
    }

    const auto touchpad_device_it = values.find("touchpad.event_device");
    if (touchpad_device_it == values.end() || touchpad_device_it->second.empty()) {
        std::cerr << "controller_id.yamlにtouchpad.event_deviceがありません\n";
        return false;
    }
    mapping.touchpad_event_device = touchpad_device_it->second;

    unsigned long touchpad_button_code = 0;
    if (!parse_number(
            "touchpad.button_code",
            std::numeric_limits<uint16_t>::max(),
            touchpad_button_code))
    {
        std::cerr << "controller_id.yamlにtouchpad.button_codeがないか、値が不正です\n";
        return false;
    }
    mapping.touchpad_button_code = static_cast<uint16_t>(touchpad_button_code);

    return true;
}
