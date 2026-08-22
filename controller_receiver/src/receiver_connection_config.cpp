#include "receiver_connection_config.hpp"

#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>

namespace
{
std::string trim(const std::string& value)
{
    const auto first = value.find_first_not_of(" \t\r");
    if (first == std::string::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(" \t\r");
    return value.substr(first, last - first + 1);
}

std::string unquote(const std::string& value)
{
    if (value.size() >= 2 && ((value.front() == '"' && value.back() == '"') ||
                              (value.front() == '\'' && value.back() == '\''))) {
        return value.substr(1, value.size() - 2);
    }
    return value;
}
} // namespace

bool load_receiver_connection_config(const char* path, ReceiverConnectionConfig& config)
{
    std::ifstream file(path);
    if (!file) {
        std::cerr << "受信設定ファイルを開けません: " << path << '\n';
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
            std::cerr << "設定ファイルの形式が不正です: " << line << '\n';
            return false;
        }

        const std::string key = trim(line.substr(0, separator));
        const std::string value = trim(line.substr(separator + 1));
        if (value.empty()) {
            section = key;
            continue;
        }

        std::string full_key = section.empty() ? key : (section + "." + key);
        values[full_key] = unquote(value);
    }

    if (values.find("wifi.port") != values.end()) {
        try {
            size_t parsed = 0;
            unsigned long port = std::stoul(values["wifi.port"], &parsed);
            if (parsed != values["wifi.port"].size() || port > std::numeric_limits<uint16_t>::max()) {
                throw std::out_of_range("port");
            }
            config.wifi_port = static_cast<uint16_t>(port);
        } catch (const std::exception&) {
            std::cerr << "wifi.port の値が不正です: " << values["wifi.port"] << '\n';
            return false;
        }
    }
    if (values.find("bluetooth.channel") != values.end()) {
        try {
            size_t parsed = 0;
            unsigned long channel = std::stoul(values["bluetooth.channel"], &parsed);
            if (parsed != values["bluetooth.channel"].size() || channel > std::numeric_limits<uint8_t>::max()) {
                throw std::out_of_range("channel");
            }
            config.bluetooth_channel = static_cast<uint8_t>(channel);
        } catch (const std::exception&) {
            std::cerr << "bluetooth.channel の値が不正です: " << values["bluetooth.channel"] << '\n';
            return false;
        }
    }

    return true;
}
