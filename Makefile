CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -pthread
TARGET := lab2
SRCS := main.cpp shared.cpp threads.cpp

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all clean
