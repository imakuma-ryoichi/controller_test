#include "comm_manager.hpp"
#include "controller_data.hpp"

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/joy.hpp>

#include <arpa/inet.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>

namespace
{
constexpr int INVALID_FD = -1;

struct ReceiverConfig
{
    uint16_t wifi_port = 5000;
    uint8_t bluetooth_channel = 1;
};

std::string system_error(const std::string& prefix)
{
    return prefix + ": " + std::strerror(errno);
}

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

void close_fd(int& fd)
{
    if (fd >= 0) {
        close(fd);
        fd = INVALID_FD;
    }
}

void set_nonblocking(int fd)
{
    const int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        throw std::runtime_error(system_error("failed to set nonblocking mode"));
    }
}

ReceiverConfig load_receiver_config(const std::string& path)
{
    ReceiverConfig config{};
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("failed to open receiver config: " + path);
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
            throw std::runtime_error("invalid receiver config line: " + line);
        }

        const std::string key = trim(line.substr(0, separator));
        const std::string value = trim(line.substr(separator + 1));
        if (value.empty()) {
            section = key;
            continue;
        }

        values[section.empty() ? key : (section + "." + key)] = unquote(value);
    }

    const auto port_it = values.find("wifi.port");
    if (port_it != values.end()) {
        const int port = std::stoi(port_it->second);
        if (port <= 0 || port > 65535) {
            throw std::runtime_error("wifi.port must be between 1 and 65535");
        }
        config.wifi_port = static_cast<uint16_t>(port);
    }

    const auto channel_it = values.find("bluetooth.channel");
    if (channel_it != values.end()) {
        const int channel = std::stoi(channel_it->second);
        if (channel < 0 || channel > 255) {
            throw std::runtime_error("bluetooth.channel must be between 0 and 255");
        }
        config.bluetooth_channel = static_cast<uint8_t>(channel);
    }

    return config;
}

int open_udp_receiver(uint16_t port)
{
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        throw std::runtime_error(system_error("failed to create UDP socket"));
    }

    try {
        const int reuse = 1;
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
            throw std::runtime_error(system_error("failed to configure UDP socket"));
        }

        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port);

        if (bind(fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
            throw std::runtime_error(system_error("failed to bind UDP socket"));
        }

        set_nonblocking(fd);
    } catch (...) {
        close_fd(fd);
        throw;
    }

    return fd;
}

int open_bluetooth_server(uint8_t channel)
{
    int fd = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
    if (fd < 0) {
        throw std::runtime_error(system_error("failed to create RFCOMM socket"));
    }

    try {
        sockaddr_rc address{};
        address.rc_family = AF_BLUETOOTH;
        address.rc_channel = channel;

        if (bind(fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
            throw std::runtime_error(system_error("failed to bind RFCOMM socket"));
        }

        if (listen(fd, 1) < 0) {
            throw std::runtime_error(system_error("failed to listen on RFCOMM socket"));
        }

        set_nonblocking(fd);
    } catch (...) {
        close_fd(fd);
        throw;
    }

    return fd;
}

float normalize_axis(int32_t value)
{
    constexpr float scale = 32767.0F;
    return std::clamp(static_cast<float>(value) / scale, -1.0F, 1.0F);
}
} // namespace

class JoyPublisherNode : public rclcpp::Node
{
public:
    JoyPublisherNode()
        : Node("joy_publisher")
    {
        const auto receiver_config_path =
            declare_parameter<std::string>(
                "receiver_config_path",
                "../controller_receiver/config/receiver_connection.yaml");
        const auto comm_config_path =
            declare_parameter<std::string>("comm_config_path", "config/comm_config.yaml");

        const ReceiverConfig receiver_config = load_receiver_config(receiver_config_path);
        comm_manager_ = std::make_shared<CommManager>(comm_config_path);

        try {
            wifi_fd_ = open_udp_receiver(receiver_config.wifi_port);
            RCLCPP_INFO(
                get_logger(),
                "UDP receiver listening on port %u",
                static_cast<unsigned int>(receiver_config.wifi_port));
        } catch (const std::exception& error) {
            RCLCPP_WARN(get_logger(), "%s", error.what());
        }

        try {
            bluetooth_server_fd_ =
                open_bluetooth_server(receiver_config.bluetooth_channel);
            RCLCPP_INFO(
                get_logger(),
                "RFCOMM receiver listening on channel %u",
                static_cast<unsigned int>(receiver_config.bluetooth_channel));
        } catch (const std::exception& error) {
            RCLCPP_WARN(get_logger(), "%s", error.what());
        }

        if (wifi_fd_ < 0 && bluetooth_server_fd_ < 0) {
            throw std::runtime_error("no receiver transport could be opened");
        }

        publisher_ = create_publisher<sensor_msgs::msg::Joy>("/controller/joy", 10);
        timer_ = create_wall_timer(
            comm_manager_->poll_period(),
            std::bind(&JoyPublisherNode::loop, this));
    }

    ~JoyPublisherNode() override
    {
        close_fd(bluetooth_client_fd_);
        close_fd(bluetooth_server_fd_);
        close_fd(wifi_fd_);
    }

private:
    void loop()
    {
        ControllerData data{};
        bool received_wifi = false;
        bool received_bluetooth = false;

        if (receive_wifi(data)) {
            received_wifi = true;
            latest_wifi_data_ = data;
            wifi_last_seen_ = CommManager::Clock::now();
        }

        if (receive_bluetooth(data)) {
            received_bluetooth = true;
            latest_bluetooth_data_ = data;
            bluetooth_last_seen_ = CommManager::Clock::now();
        }

        const auto selected = comm_manager_->select_channel(
            CommManager::Clock::now(),
            wifi_last_seen_,
            bluetooth_last_seen_);

        if (!selected.has_value()) {
            return;
        }

        const ControllerData* selected_data = nullptr;
        if (*selected == CommManager::Channel::WiFi && latest_wifi_data_.has_value()) {
            selected_data = &*latest_wifi_data_;
        } else if (
            *selected == CommManager::Channel::Bluetooth &&
            latest_bluetooth_data_.has_value())
        {
            selected_data = &*latest_bluetooth_data_;
        }

        if (selected_data == nullptr) {
            return;
        }

        if (comm_manager_->publish_on_new_data_only()) {
            const bool selected_channel_updated =
                (*selected == CommManager::Channel::WiFi && received_wifi) ||
                (*selected == CommManager::Channel::Bluetooth && received_bluetooth);

            if (!selected_channel_updated) {
                return;
            }
        }

        publish_joy(*selected_data);
    }

    bool receive_wifi(ControllerData& latest)
    {
        if (wifi_fd_ < 0) {
            return false;
        }

        bool received = false;
        while (true) {
            ControllerData data{};
            const ssize_t size = recv(wifi_fd_, &data, sizeof(data), MSG_DONTWAIT);

            if (size == static_cast<ssize_t>(sizeof(data))) {
                latest = data;
                received = true;
                continue;
            }

            if (size < 0 && errno == EINTR) {
                continue;
            }
            if (size < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                break;
            }
            break;
        }

        return received;
    }

    bool receive_bluetooth(ControllerData& latest)
    {
        accept_bluetooth_client();

        if (bluetooth_client_fd_ < 0) {
            return false;
        }

        bool received = false;
        while (true) {
            const ssize_t size = recv(
                bluetooth_client_fd_,
                bluetooth_buffer_.data() + bluetooth_buffer_size_,
                bluetooth_buffer_.size() - bluetooth_buffer_size_,
                MSG_DONTWAIT);

            if (size > 0) {
                bluetooth_buffer_size_ += static_cast<std::size_t>(size);
                if (bluetooth_buffer_size_ == bluetooth_buffer_.size()) {
                    std::memcpy(&latest, bluetooth_buffer_.data(), sizeof(latest));
                    bluetooth_buffer_size_ = 0;
                    received = true;
                }
                continue;
            }

            if (size == 0) {
                disconnect_bluetooth_client();
                break;
            }
            if (errno == EINTR) {
                continue;
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }

            disconnect_bluetooth_client();
            break;
        }

        return received;
    }

    void accept_bluetooth_client()
    {
        if (bluetooth_server_fd_ < 0 || bluetooth_client_fd_ >= 0) {
            return;
        }

        sockaddr_rc client{};
        socklen_t client_size = sizeof(client);
        const int fd = accept(
            bluetooth_server_fd_,
            reinterpret_cast<sockaddr*>(&client),
            &client_size);

        if (fd < 0) {
            return;
        }

        try {
            set_nonblocking(fd);
        } catch (const std::exception& error) {
            int mutable_fd = fd;
            close_fd(mutable_fd);
            RCLCPP_WARN(get_logger(), "%s", error.what());
            return;
        }

        bluetooth_client_fd_ = fd;
        bluetooth_buffer_size_ = 0;
        RCLCPP_INFO(get_logger(), "RFCOMM client connected");
    }

    void disconnect_bluetooth_client()
    {
        close_fd(bluetooth_client_fd_);
        bluetooth_buffer_size_ = 0;
        bluetooth_last_seen_.reset();
        latest_bluetooth_data_.reset();
        RCLCPP_WARN(get_logger(), "RFCOMM client disconnected");
    }

    void publish_joy(const ControllerData& data)
    {
        sensor_msgs::msg::Joy joy;
        joy.header.stamp = now();
        joy.header.frame_id = "controller";

        joy.axes.reserve(data.axes.size());
        for (const int32_t value : data.axes) {
            joy.axes.push_back(normalize_axis(value));
        }

        joy.buttons.reserve(data.buttons.size());
        for (const int32_t value : data.buttons) {
            joy.buttons.push_back(value != 0 ? 1 : 0);
        }

        publisher_->publish(joy);
    }

    int wifi_fd_ = INVALID_FD;
    int bluetooth_server_fd_ = INVALID_FD;
    int bluetooth_client_fd_ = INVALID_FD;
    std::array<uint8_t, sizeof(ControllerData)> bluetooth_buffer_{};
    std::size_t bluetooth_buffer_size_ = 0;

    std::shared_ptr<CommManager> comm_manager_;
    std::optional<ControllerData> latest_wifi_data_;
    std::optional<ControllerData> latest_bluetooth_data_;
    std::optional<CommManager::TimePoint> wifi_last_seen_;
    std::optional<CommManager::TimePoint> bluetooth_last_seen_;

    rclcpp::Publisher<sensor_msgs::msg::Joy>::SharedPtr publisher_;
    rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char* argv[])
{
    rclcpp::init(argc, argv);

    try {
        rclcpp::spin(std::make_shared<JoyPublisherNode>());
    } catch (const std::exception& error) {
        std::cerr << "joy_publisher failed: " << error.what() << '\n';
        rclcpp::shutdown();
        return 1;
    }

    rclcpp::shutdown();
    return 0;
}
