#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <vector>

class CommManager
{
public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    enum class Channel
    {
        WiFi,
        Bluetooth
    };

    explicit CommManager(const std::string& config_path);

    std::optional<Channel> select_channel(
        TimePoint now,
        const std::optional<TimePoint>& wifi_last_seen,
        const std::optional<TimePoint>& bluetooth_last_seen) const;

    static const char* channel_name(Channel channel);

private:
    bool is_active(
        TimePoint now,
        const std::optional<TimePoint>& last_seen) const;

    std::vector<Channel> priority_;
    std::chrono::milliseconds stale_timeout_{200};
};
