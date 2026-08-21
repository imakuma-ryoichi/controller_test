CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -O2 -Iinclude
LDFLAGS := -lbluetooth

TARGET := build/controller_receiver

SOURCES := \
	src/main.cpp \
	src/wifi_receiver.cpp \
	src/bluetooth_receiver.cpp

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SOURCES)
	mkdir -p build
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET) $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf build
