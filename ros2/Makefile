ROS_DISTRO ?= jazzy
ROS_PATH := /opt/ros/$(ROS_DISTRO)

CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -O2 \
            -Iinclude \
            -Isrc \
            -I../controller_sender/include \
            -I../controller_receiver/include \
            -I$(ROS_PATH)/include/rclcpp \
            -I$(ROS_PATH)/include/sensor_msgs \
            -I$(ROS_PATH)/include/rcutils \
            -I$(ROS_PATH)/include/rmw \
            -I$(ROS_PATH)/include/builtin_interfaces \
            -I$(ROS_PATH)/include/rosidl_runtime_c \
            -I$(ROS_PATH)/include/rosidl_runtime_cpp \
            -I$(ROS_PATH)/include/tracetools \
            -I$(ROS_PATH)/include/rcl \
            -I$(ROS_PATH)/include/rcl_interfaces \
            -I$(ROS_PATH)/include/rcpputils

LDFLAGS := -L$(ROS_PATH)/lib -Wl,-rpath,$(ROS_PATH)/lib
LDLIBS := -lrclcpp -lsensor_msgs__rosidl_typesupport_cpp -lsensor_msgs__rosidl_generator_c -lbluetooth -pthread

TARGET := build/joy_publisher

SOURCES := $(wildcard src/*.cpp)
OBJECTS := $(SOURCES:src/%.cpp=build/%.o)

.PHONY: all build launch clean

all: $(TARGET)

build: $(TARGET)

$(TARGET): $(OBJECTS)
	@mkdir -p $(dir $@)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS) $(LDLIBS)
	chmod +x $@

build/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

launch: $(TARGET)
	ros2 launch launch/joy_publisher.launch.py

clean:
	rm -rf build
