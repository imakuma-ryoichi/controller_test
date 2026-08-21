CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -O2 -Iinclude
LDLIBS := -lbluetooth

TARGET := build/controller_sender

SOURCES := $(wildcard src/*.cpp)
OBJECTS := $(SOURCES:src/%.cpp=build/%.o)

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJECTS)
	mkdir -p $(dir $@)
	$(CXX) $(OBJECTS) -o $@ $(LDLIBS)
	chmod +x $@

build/%.o: src/%.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf build
