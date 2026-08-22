#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/joy.hpp>
#include "comm_manager.hpp"

// Placeholder for the data structure used by the existing codebase.
struct ControllerData {
  std::vector<float> axes;
  std::vector<int> buttons;
};

// Placeholder receive functions – replace with actual implementations.
bool recv_wifi(int fd, ControllerData &data) { /* TODO */ return false; }
bool recv_bluetooth(int fd, ControllerData &data) { /* TODO */ return false; }

class JoyPublisherNode : public rclcpp::Node {
public:
  JoyPublisherNode()
  : Node("joy_publisher") {
    // Parameters (could be made ROS2 parameters)
    std::string config_path = "/home/user/controller-rox/ros2/config/comm_config.yaml";
    const char* wifi_address = "192.168.1.100";
    uint16_t wifi_port = 5000;
    const char* bt_address = "XX:XX:XX:XX:XX:XX";
    uint8_t bt_channel = 1;

    // Initialize transports (use the same helpers as the sender side)
    wifi_fd_ = create_wifi_sender(wifi_address, wifi_port); // re‑use sender init as a placeholder for a receiver socket
    bt_fd_ = connect_bluetooth(bt_address, bt_channel);
    if (wifi_fd_ < 0 || bt_fd_ < 0) {
      RCLCPP_ERROR(this->get_logger(), "Failed to open transport sockets");
      rclcpp::shutdown();
      return;
    }

    comm_manager_ = std::make_shared<CommManager>(config_path, wifi_fd_, bt_fd_);
    publisher_ = this->create_publisher<sensor_msgs::msg::Joy>("/controller/joy", 10);
    timer_ = this->create_wall_timer(std::chrono::milliseconds(10),
                                     std::bind(&JoyPublisherNode::loop, this));
  }

private:
  void loop() {
    // Update channel health and pick the current channel inside CommManager
    // We just request the current channel via a tiny helper (exposed as public for this demo).
    // For simplicity we repeat the selection logic here.
    // Receive data from the selected channel
    ControllerData data;
    bool received = false;
    // Determine which channel is currently considered good according to priority
    // (reuse same logic as CommManager – we call its private method via a public wrapper for demo).
    // Here we perform a simple check: try Wi‑Fi first if it is good.
    // In a real implementation, CommManager would expose the chosen channel.
    if (comm_manager_->wifi_good_) {
      received = recv_wifi(wifi_fd_, data);
    }
    if (!received && comm_manager_->bt_good_) {
      received = recv_bluetooth(bt_fd_, data);
    }
    if (!received) {
      // No data available – just return.
      return;
    }

    // Convert to sensor_msgs::msg::Joy
    sensor_msgs::msg::Joy joy_msg;
    joy_msg.header.stamp = this->now();
    joy_msg.axes = data.axes;
    // Convert int buttons to int32 (Joy expects int32)
    joy_msg.buttons.resize(data.buttons.size());
    for (size_t i = 0; i < data.buttons.size(); ++i) {
      joy_msg.buttons[i] = data.buttons[i];
    }
    publisher_->publish(joy_msg);
  }

  int wifi_fd_;
  int bt_fd_;
  std::shared_ptr<CommManager> comm_manager_;
  rclcpp::Publisher<sensor_msgs::msg::Joy>::SharedPtr publisher_;
  rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char *argv[]) {
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<JoyPublisherNode>());
  rclcpp::shutdown();
  return 0;
}
