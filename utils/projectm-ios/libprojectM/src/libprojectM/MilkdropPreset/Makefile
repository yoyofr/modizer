# Makefile for testing redundant parentheses removal

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -I.
LDFLAGS =

# Source files
SRCS = test_int_assignment_fix.cpp ShaderPreprocessor.cpp HLSLTypeFixer.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = test_int_assignment_fix

# Default target
all: $(TARGET)

# Link the executable
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Compile source files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Run the test
run: $(TARGET)
	./$(TARGET)

# Clean build artifacts
clean:
	rm -f $(OBJS) $(TARGET)

# Rebuild everything
rebuild: clean all

.PHONY: all run clean rebuild
