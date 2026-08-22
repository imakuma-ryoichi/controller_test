#include "comm_manager.hpp"

#include <cctype>
#include <fstream>
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

CommManager::Channel parse_channel_name(const std::string& name)
{
    if (name == "wifi") {
        return CommManager::Channel::WiFi;
    }
    if (name == "bluetooth") {
        return CommManager::Channel::Bluetooth;
    }
    throw std::runtime_error("unknown communication channel: " + name);
}

bool parse_bool(const std::string& value, bool& result)
{
    std::string normalized;
    normalized.reserve(value.size());
    for (const char c : value) {
        normalized.push_back(static_cast<char>(
            std::tolower(static_cast<unsigned char>(c))));
    }

    if (normalized == "true" || normalized == "yes" ||
        normalized == "on" || normalized == "1")
    {
        result = true;
        return true;
    }
    if (normalized == "false" || normalized == "no" ||
        normalized == "off" || normalized == "0")
    {
        result = false;
        return true;
    }

    return false;
}

std::map<std::string, std::string> load_config_values(
    const std::string& path,
    std::vector<CommManager::Channel>& priority)
{
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("failed to open communication config: " + path);
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

        if (!line.empty() && line.front() == '-') {
            if (section != "priority") {
                throw std::runtime_error("list item is only supported in priority");
            }
            priority.push_back(parse_channel_name(trim(line.substr(1))));
            continue;
        }

        const auto separator = line.find(':');
        if (separator == std::string::npos) {
            throw std::runtime_error("invalid communication config line: " + line);
        }

        const std::string key = trim(line.substr(0, separator));
        const std::string value = trim(line.substr(separator + 1));
        if (value.empty()) {
            section = key;
            continue;
        }

        values[key] = value;
    }

    return values;
}
} // namespace

CommManager::CommManager(const std::string& config_path)
    : priority_{Channel::WiFi, Channel::Bluetooth}
{
    std::vector<Channel> configured_priority;
    const auto values = load_config_values(config_path, configured_priority);

    if (!configured_priority.empty()) {
        priority_ = configured_priority;
    }

    const auto timeout_it = values.find("stale_timeout_ms");
    if (timeout_it != values.end()) {
        const int timeout_ms = std::stoi(timeout_it->second);
        if (timeout_ms <= 0 || timeout_ms > 60000) {
            throw std::runtime_error("stale_timeout_ms must be between 1 and 60000");
        }
        stale_timeout_ = std::chrono::milliseconds(timeout_ms);
    }

    const auto poll_rate_it = values.find("poll_rate_hz");
    if (poll_rate_it != values.end()) {
        const int rate_hz = std::stoi(poll_rate_it->second);
        if (rate_hz <= 0 || rate_hz > 1000) {
            throw std::runtime_error("poll_rate_hz must be between 1 and 1000");
        }
        poll_period_ = std::chrono::milliseconds(1000 / rate_hz);
    }

    const auto publish_it = values.find("publish_on_new_data_only");
    if (publish_it != values.end() &&
        !parse_bool(publish_it->second, publish_on_new_data_only_))
    {
        throw std::runtime_error(
            "publish_on_new_data_only must be true or false");
    }
}

std::optional<CommManager::Channel> CommManager::select_channel(
    TimePoint now,
    const std::optional<TimePoint>& wifi_last_seen,
    const std::optional<TimePoint>& bluetooth_last_seen) const
{
    for (const Channel channel : priority_) {
        if (channel == Channel::WiFi && is_active(now, wifi_last_seen)) {
            return channel;
        }
        if (channel == Channel::Bluetooth && is_active(now, bluetooth_last_seen)) {
            return channel;
        }
    }

    return std::nullopt;
}

std::chrono::milliseconds CommManager::poll_period() const
{
    return poll_period_;
}

bool CommManager::publish_on_new_data_only() const
{
    return publish_on_new_data_only_;
}

const char* CommManager::channel_name(Channel channel)
{
    switch (channel) {
    case Channel::WiFi:
        return "wifi";
    case Channel::Bluetooth:
        return "bluetooth";
    }
    return "unknown";
}

bool CommManager::is_active(
    TimePoint now,
    const std::optional<TimePoint>& last_seen) const
{
    return last_seen.has_value() && now - *last_seen <= stale_timeout_;
}
