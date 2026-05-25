# Challenge 3 Makefile

# Compiler
CXX		 ?= g++
# Compiler flags
## C++ Standard 23, Optimization, Additional warnings, non-standard code
CXXFLAGS ?= -std=c++23 -O3 -Wall -Wextra -pedantic
# Preprocessor flags
CPPFLAGS ?= -Iinclude
LDLIBS 	 ?= 
# Use C++ linker.
LINK.o   := $(LINK.cc)


# Variables
SRC_DIR = src/
BUILD_DIR = build/
BIN_DIR = bin/

SRCS = $(wildcard $(SRC_DIR)*.cpp)  # All source files
HEADERS=$(wildcard include/*.hpp) # Get all headers
OBJS = $(SRCS:$(SRC_DIR)%.cpp=$(BUILD_DIR)%.o) # All compiled .cpp files in build/
EXEC = $(BIN_DIR)main # Executable

# Build executable when running 'make'
all: $(EXEC)

.PHONY: all clean distclean

clean:
	$(RM) $(OBJS)

distclean: clean
	$(RM) $(EXEC)

# Compile
$(BUILD_DIR)%.o: $(SRC_DIR)%.cpp
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

# Link objects
$(EXEC): $(OBJS)
	mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(OBJS) -o $(EXEC) $(LDLIBS)