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

    std::map<std::string, unsigned int> values;
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

        if (section.empty() || key == "send_rate_hz") {
            if (key == "send_rate_hz") {
                try {
                    size_t parsed = 0;
                    unsigned long rate = std::stoul(value, &parsed);
                    if (parsed != value.size() || rate == 0 || rate > 10000) {
                        throw std::out_of_range("send_rate_hz");
                    }
                    mapping.send_rate_hz = static_cast<uint32_t>(rate);
                } catch (const std::exception&) {
                    std::cerr << "controller_id.yamlのsend_rate_hzが不正です: " << value << '\n';
                    return false;
                }
                continue;
            }
        }

        if (section != "axis" && section != "button") {
            std::cerr << "controller_id.yamlのセクションが不正です: "
                      << section << '\n';
            return false;
        }

        unsigned int number = 0;
        try {
            size_t parsed = 0;
            number = std::stoul(value, &parsed);
            if (parsed != value.size() || number > std::numeric_limits<uint8_t>::max()) {
                throw std::out_of_range("event number");
            }
        } catch (const std::exception&) {
            std::cerr << "controller_id.yamlの値が不正です: " << key
                      << ": " << value << '\n';
            return false;
        }
        values[section + "." + key] = number;
    }

    for (size_t index = 0; index < AXIS_NAMES.size(); ++index) {
        const auto it = values.find("axis." + std::string(AXIS_NAMES[index]));
        if (it == values.end()) {
            std::cerr << "controller_id.yamlにaxis." << AXIS_NAMES[index]
                      << "がありません\n";
            return false;
        }
        mapping.axis_events[index] = static_cast<uint8_t>(it->second);
    }
    for (size_t index = 0; index < BUTTON_NAMES.size(); ++index) {
        const auto it = values.find("button." + std::string(BUTTON_NAMES[index]));
        if (it == values.end()) {
            std::cerr << "controller_id.yamlにbutton." << BUTTON_NAMES[index]
                      << "がありません\n";
            return false;
        }
        mapping.button_events[index] = static_cast<uint8_t>(it->second);
    }
    return true;
}
