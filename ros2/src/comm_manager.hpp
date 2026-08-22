#pragma once

#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>

struct ControllerData; // forward declaration of existing data struct

class CommManager {
public:
    // config_path: path to comm_config.yaml
    // wifi_fd and bt_fd are file descriptors for the underlying transports
    CommManager(const std::string& config_path, int wifi_fd, int bt_fd);

    // Send data using the selected channel according to priority and link quality
    void send(const ControllerData& data);

private:
    enum class Channel { WiFi, Bluetooth };

    // Configuration
    std::vector<Channel> priority_; // ordered list of preferred channels
    struct Thresholds { int bad_rssi; int recover_rssi; } wifi_thr_, bt_thr_;

    // Runtime state
    int wifi_fd_;
    int bt_fd_;
    Channel current_channel_;
    bool wifi_good_ = true;
    bool bt_good_ = true;

    // Helper functions to obtain RSSI (placeholder implementations)
    int get_wifi_rssi();
    int get_bt_rssi();

    void update_channel_state();
    void send_wifi(const ControllerData& data);
    void send_bluetooth(const ControllerData& data);
};
