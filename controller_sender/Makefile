CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -O2 -Iinclude
LDLIBS := -lbluetooth -pthread

TARGET := build/controller_sender

SOURCES := $(wildcard src/*.cpp)
OBJECTS := $(SOURCES:src/%.cpp=build/%.o)

.PHONY: all clean run install

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@mkdir -p $(dir $@)
	$(CXX) $(OBJECTS) -o $@ $(LDLIBS)

build/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

install: $(TARGET)
	install -D -m 755 $(TARGET) /usr/local/bin/controller_sender
	install -D -m 644 config/controller_id.yaml /etc/controller_sender/controller_id.yaml
	install -D -m 644 config/controller_connection.yaml /etc/controller_sender/controller_connection.yaml

clean:
	rm -rf build
