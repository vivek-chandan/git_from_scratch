# Compiler and Flags
CXX = g++
CXXFLAGS = -Wall -std=c++23

# OpenSSL paths
OPENSSL_ROOT_DIR = /usr
OPENSSL_INCLUDE_DIR = $(OPENSSL_ROOT_DIR)/include
OPENSSL_LIB_DIR = $(OPENSSL_ROOT_DIR)/lib/x86_64-linux-gnu
LIBS = -lssl -lcrypto -lz

# Source and target definitions
TARGET = mygit
SRC_DIR = .  # Current directory as source
SOURCES = $(shell find $(SRC_DIR) -name '*.cpp')
OBJECTS = $(SOURCES:.cpp=.o)

# Default target to build the executable
all: $(TARGET)

# Link object files to create the executable
$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET) -L$(OPENSSL_LIB_DIR) $(LIBS)

# Compile source files to object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -I$(OPENSSL_INCLUDE_DIR) -c $< -o $@

# Clean build files
clean:
	rm -f $(OBJECTS) $(TARGET)

# Additional help command
help:
	@echo "Usage:"
	@echo "  ./mygit <command>  Build and run the executable with specified command"
	@echo "  make               Build the project"
	@echo "  make clean         Remove build artifacts"
	@echo "  make help          Show this help message"
