CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -O2

TARGET := controller_sender
SOURCE := src/controller_sender.cpp

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CXX) $(CXXFLAGS) $(SOURCE) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
