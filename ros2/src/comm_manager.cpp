#include "comm_manager.hpp"
#include <yaml-cpp/yaml.h>
#include <iostream>

CommManager::CommManager(const std::string& config_path, int wifi_fd, int bt_fd)
    : wifi_fd_(wifi_fd), bt_fd_(bt_fd) {
    // Load YAML
    YAML::Node cfg = YAML::LoadFile(config_path);
    // priority order
    for (const auto& ch : cfg["priority"]) {
        std::string name = ch.as<std::string>();
        if (name == "wifi") priority_.push_back(Channel::WiFi);
        else if (name == "bluetooth") priority_.push_back(Channel::Bluetooth);
    }
    // thresholds
    auto thr = cfg["thresholds"];
    wifi_thr_.bad_rssi = thr["wifi"]["bad_rssi"].as<int>();
    wifi_thr_.recover_rssi = thr["wifi"]["recover_rssi"].as<int>();
    bt_thr_.bad_rssi = thr["bluetooth"]["bad_rssi"].as<int>();
    bt_thr_.recover_rssi = thr["bluetooth"]["recover_rssi"].as<int>();

    // Initial state assumes good
    wifi_good_ = true;
    bt_good_ = true;
    // start with highest priority that is good
    update_channel_state();
}

int CommManager::get_wifi_rssi() {
    // TODO: replace with actual RSSI retrieval. Placeholder returns 0 (good).
    return 0;
}

int CommManager::get_bt_rssi() {
    // TODO: replace with actual RSSI retrieval. Placeholder returns 0.
    return 0;
}

void CommManager::update_channel_state() {
    int wifi_rssi = get_wifi_rssi();
    int bt_rssi = get_bt_rssi();
    wifi_good_ = (wifi_rssi >= wifi_thr_.bad_rssi);
    bt_good_ = (bt_rssi >= bt_thr_.bad_rssi);

    // Choose channel based on priority and good state
    for (auto ch : priority_) {
        if (ch == Channel::WiFi && wifi_good_) { current_channel_ = Channel::WiFi; return; }
        if (ch == Channel::Bluetooth && bt_good_) { current_channel_ = Channel::Bluetooth; return; }
    }
    // Fallback: keep previous channel
}

void CommManager::send_wifi(const ControllerData& data) {
    // Existing function signature: send_wifi(int fd, const ControllerData&)
    // Assuming external function is visible via include elsewhere.
    extern void send_wifi(int fd, const ControllerData&);
    send_wifi(wifi_fd_, data);
}

void CommManager::send_bluetooth(const ControllerData& data) {
    extern void send_bluetooth(int fd, const ControllerData&);
    send_bluetooth(bt_fd_, data);
}

void CommManager::send(const ControllerData& data) {
    // Update channel health
    update_channel_state();
    // If current channel became bad, switch according to priority
    if (current_channel_ == Channel::WiFi && !wifi_good_) {
        // find next good channel
        for (auto ch : priority_) {
            if (ch == Channel::Bluetooth && bt_good_) { current_channel_ = Channel::Bluetooth; break; }
        }
    } else if (current_channel_ == Channel::Bluetooth && !bt_good_) {
        for (auto ch : priority_) {
            if (ch == Channel::WiFi && wifi_good_) { current_channel_ = Channel::WiFi; break; }
        }
    }
    // Send via selected channel
    if (current_channel_ == Channel::WiFi) {
        send_wifi(data);
    } else {
        send_bluetooth(data);
    }
}
