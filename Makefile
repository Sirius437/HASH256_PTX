# Makefile for SHA256 PTX Implementation
# Author: HashRuda Project
# Date: 2025-11-06

# Compiler settings
CXX = g++
CXXFLAGS = -O3 -std=c++11 -Wall
CUDA_PATH = /usr/local/cuda-12.8
INCLUDES = -I./include -I$(CUDA_PATH)/include
LDFLAGS = -L$(CUDA_PATH)/lib64 -lcuda -lcudart

# Directories
SRC_DIR = src
TEST_DIR = tests
PTX_DIR = ptx
BUILD_DIR = build

# Source files
CPU_SOURCES = $(SRC_DIR)/sha256.cpp
TEST_SOURCES = $(TEST_DIR)/test_ptx_sha256.cpp
PTX_KERNEL = $(PTX_DIR)/sha256_kernel_full.ptx

# Output
TEST_BIN = test_ptx_sha256

# Targets
.PHONY: all clean test ptx help

all: ptx $(TEST_BIN)

# Generate PTX kernel
ptx: $(PTX_KERNEL)

$(PTX_KERNEL): generate_sha256_ptx.py
	@echo "Generating PTX kernel..."
	@mkdir -p $(PTX_DIR)
	@python3 generate_sha256_ptx.py
	@echo "✓ PTX kernel generated: $(PTX_KERNEL)"

# Build test program
$(TEST_BIN): $(TEST_SOURCES) $(CPU_SOURCES) $(PTX_KERNEL)
	@echo "Compiling test program..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(TEST_SOURCES) $(CPU_SOURCES) -o $(TEST_BIN) $(LDFLAGS)
	@echo "✓ Test program built: $(TEST_BIN)"

# Run tests
test: $(TEST_BIN)
	@echo ""
	@echo "Running SHA256 PTX tests..."
	@echo "=============================="
	@./$(TEST_BIN)

# Generate reference values for debugging
reference:
	@echo "Computing SHA256 reference values..."
	@python3 compute_sha256_reference.py

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	@rm -f $(TEST_BIN)
	@rm -f $(BUILD_DIR)/*.o
	@echo "✓ Clean complete"

# Clean everything including generated PTX
distclean: clean
	@echo "Removing generated PTX..."
	@rm -f $(PTX_KERNEL)
	@echo "✓ Deep clean complete"

# Help target
help:
	@echo "SHA256 PTX Implementation - Makefile"
	@echo ""
	@echo "Available targets:"
	@echo "  make          - Generate PTX and build test program (default)"
	@echo "  make ptx      - Generate PTX kernel only"
	@echo "  make test     - Build and run test suite"
	@echo "  make reference- Compute reference SHA256 values"
	@echo "  make clean    - Remove build artifacts"
	@echo "  make distclean- Remove all generated files"
	@echo "  make help     - Show this help message"
	@echo ""
	@echo "Requirements:"
	@echo "  - NVIDIA GPU with CUDA 12.0+"
	@echo "  - Python 3.6+"
	@echo "  - g++ with C++11 support"
