ROS_DISTRO ?= jazzy
ROS_PATH := /opt/ros/$(ROS_DISTRO)
PKG_CONFIG ?= pkg-config
ROS_PKG_CONFIG_PACKAGES := rclcpp sensor_msgs
ROS_PKG_CFLAGS := $(shell PKG_CONFIG_PATH="$(ROS_PATH)/lib/pkgconfig:$$PKG_CONFIG_PATH" $(PKG_CONFIG) --cflags $(ROS_PKG_CONFIG_PACKAGES) 2>/dev/null)
ROS_PKG_LIBS := $(shell PKG_CONFIG_PATH="$(ROS_PATH)/lib/pkgconfig:$$PKG_CONFIG_PATH" $(PKG_CONFIG) --libs $(ROS_PKG_CONFIG_PACKAGES) 2>/dev/null)
ROS_INCLUDE_DIRS := -I$(ROS_PATH)/include \
                    $(foreach dir,$(wildcard $(ROS_PATH)/include/*),-I$(dir))
ROS_FALLBACK_LIBS := -lrclcpp \
                     -lrcl \
                     -lrcutils \
                     -lrcl_yaml_param_parser \
                     -ltracetools \
                     -lrmw \
                     -lrmw_implementation \
                     -lrosidl_runtime_c \
                     -lsensor_msgs__rosidl_typesupport_cpp \
                     -lsensor_msgs__rosidl_generator_c

CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -O2 \
            -Iinclude \
            -Isrc \
            -I../controller_sender/include \
            -I../controller_receiver/include \
            $(ROS_INCLUDE_DIRS) \
            $(ROS_PKG_CFLAGS)

LDFLAGS := -L$(ROS_PATH)/lib -Wl,-rpath,$(ROS_PATH)/lib
LDLIBS := $(if $(strip $(ROS_PKG_LIBS)),$(ROS_PKG_LIBS),$(ROS_FALLBACK_LIBS)) -lbluetooth -pthread

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
