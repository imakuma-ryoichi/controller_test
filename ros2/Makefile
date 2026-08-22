ROS_DISTRO ?= jazzy
ROS_PATH := /opt/ros/$(ROS_DISTRO)
ROS_INCLUDE_DIRS := -I$(ROS_PATH)/include \
                    $(foreach dir,$(wildcard $(ROS_PATH)/include/*),-I$(dir))

CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -O2 \
            -Iinclude \
            -Isrc \
            -I../controller_sender/include \
            -I../controller_receiver/include \
            $(ROS_INCLUDE_DIRS)

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
