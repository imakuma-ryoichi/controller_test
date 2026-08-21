CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -O2

TARGET := build/controller_sender
SOURCE := src/main.cpp

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CXX) $(CXXFLAGS) $(SOURCE) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
