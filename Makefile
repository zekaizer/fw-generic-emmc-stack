# eMMC Generic Stack Makefile
# Supports function-level linking for dead code elimination

# Compiler and tools
CC ?= gcc
AR ?= ar
SIZE ?= size
OBJDUMP ?= objdump

# Project configuration  
PROJECT_NAME = emmc_stack
VERSION = 1.0.0

# Directories
SRC_DIR = src
BUILD_DIR = build
INCLUDE_DIR = $(SRC_DIR)/include
HAL_DIR = $(SRC_DIR)/hal
DRIVER_DIR = $(SRC_DIR)/driver  
PROTOCOL_DIR = $(SRC_DIR)/protocol
EXAMPLES_DIR = examples
TESTS_DIR = tests

# Source files
CORE_SOURCES = \
	$(DRIVER_DIR)/emmc_core.c \
	$(PROTOCOL_DIR)/emmc_protocol.c

# Advanced features (linked only if used)
ADVANCED_SOURCES = \
	$(PROTOCOL_DIR)/emmc_advanced.c

# All sources
ALL_SOURCES = $(CORE_SOURCES) $(ADVANCED_SOURCES)

# Test and example sources
EXAMPLE_SOURCES = $(EXAMPLES_DIR)/basic_usage.c
TEST_SOURCES = $(TESTS_DIR)/test_rpmb.c
HAL_STUB_SOURCES = $(TESTS_DIR)/hal_stub.c

# Include paths
INCLUDES = -I$(INCLUDE_DIR) -I$(HAL_DIR) -I$(DRIVER_DIR) -I$(PROTOCOL_DIR)

# Compiler flags
CFLAGS = -std=c99 -Wall -Wextra -Werror
CFLAGS += -ffunction-sections -fdata-sections -fno-common
CFLAGS += $(INCLUDES)

# Linker flags for dead code elimination
LDFLAGS = -Wl,--gc-sections -Wl,--print-gc-sections

# Build type specific flags
ifeq ($(BUILD_TYPE),debug)
    CFLAGS += -O0 -g3 -DDEBUG
    BUILD_SUFFIX = _debug
else ifeq ($(BUILD_TYPE),minimal)
    CFLAGS += -Os -DNDEBUG -DEMMC_MINIMAL_BUILD
    BUILD_SUFFIX = _minimal
else
    CFLAGS += -O2 -DNDEBUG
    BUILD_SUFFIX = 
endif

# Optional LTO
ifeq ($(LTO),1)
    CFLAGS += -flto
    LDFLAGS += -flto
    BUILD_SUFFIX := $(BUILD_SUFFIX)_lto
endif

# Optional bare-metal flags
ifeq ($(BARE_METAL),1)
    CFLAGS += -nostdlib -ffreestanding -DEMMC_BARE_METAL=1
    BUILD_SUFFIX := $(BUILD_SUFFIX)_baremetal
endif

# ARM target flags
ifeq ($(ARM_TARGET),1)
    CFLAGS += -mcpu=cortex-a7 -mthumb -DEMMC_ARM_TARGET=1
    BUILD_SUFFIX := $(BUILD_SUFFIX)_arm
endif

# Object files
CORE_OBJECTS = $(CORE_SOURCES:%.c=$(BUILD_DIR)/%$(BUILD_SUFFIX).o)
ADVANCED_OBJECTS = $(ADVANCED_SOURCES:%.c=$(BUILD_DIR)/%$(BUILD_SUFFIX).o)
ALL_OBJECTS = $(CORE_OBJECTS) $(ADVANCED_OBJECTS)

# Target library names
FULL_LIB = $(BUILD_DIR)/lib$(PROJECT_NAME)$(BUILD_SUFFIX).a
MINIMAL_LIB = $(BUILD_DIR)/lib$(PROJECT_NAME)_minimal$(BUILD_SUFFIX).a

# Phony targets
.PHONY: all clean distclean full minimal examples tests help size info

# Default target
all: full

# Full featured library (includes advanced functions)
full: $(FULL_LIB)

$(FULL_LIB): $(ALL_OBJECTS) | $(BUILD_DIR)
	@echo "Creating full library: $@"
	$(AR) rcs $@ $^
	@echo "Library created successfully"

# Minimal library (core functions only)  
minimal: $(MINIMAL_LIB)

$(MINIMAL_LIB): $(CORE_OBJECTS) | $(BUILD_DIR)
	@echo "Creating minimal library: $@"
	$(AR) rcs $@ $^
	@echo "Minimal library created successfully"

# Object file compilation
$(BUILD_DIR)/%$(BUILD_SUFFIX).o: %.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	@echo "Compiling: $<"
	$(CC) $(CFLAGS) -c $< -o $@

# Create build directory
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(BUILD_DIR)/$(DRIVER_DIR)
	@mkdir -p $(BUILD_DIR)/$(PROTOCOL_DIR)
	@mkdir -p $(BUILD_DIR)/$(EXAMPLES_DIR)
	@mkdir -p $(BUILD_DIR)/$(TESTS_DIR)

# Examples
examples: $(BUILD_DIR)/emmc_example$(BUILD_SUFFIX) $(BUILD_DIR)/emmc_minimal_example$(BUILD_SUFFIX)

$(BUILD_DIR)/emmc_example$(BUILD_SUFFIX): $(EXAMPLE_SOURCES) $(HAL_STUB_SOURCES) $(FULL_LIB)
	@echo "Building full example: $@"
	$(CC) $(CFLAGS) $(EXAMPLE_SOURCES) $(HAL_STUB_SOURCES) -L$(BUILD_DIR) -l$(PROJECT_NAME)$(BUILD_SUFFIX) $(LDFLAGS) -o $@
	@echo "Example built successfully"

$(BUILD_DIR)/emmc_minimal_example$(BUILD_SUFFIX): $(EXAMPLE_SOURCES) $(HAL_STUB_SOURCES) $(MINIMAL_LIB)
	@echo "Building minimal example: $@"  
	$(CC) $(CFLAGS) $(EXAMPLE_SOURCES) $(HAL_STUB_SOURCES) -L$(BUILD_DIR) -l$(PROJECT_NAME)_minimal$(BUILD_SUFFIX) $(LDFLAGS) -o $@
	@echo "Minimal example built successfully"

# Tests
tests: $(BUILD_DIR)/emmc_test_rpmb$(BUILD_SUFFIX)

$(BUILD_DIR)/emmc_test_rpmb$(BUILD_SUFFIX): $(TEST_SOURCES) $(HAL_STUB_SOURCES) $(FULL_LIB)
	@echo "Building RPMB test: $@"
	$(CC) $(CFLAGS) $(TEST_SOURCES) $(HAL_STUB_SOURCES) -L$(BUILD_DIR) -l$(PROJECT_NAME)$(BUILD_SUFFIX) $(LDFLAGS) -o $@
	@echo "Test built successfully"

# Size analysis
size: full minimal
	@echo "=== Size Comparison ==="
	@echo "Full Library:"
	@$(SIZE) $(FULL_LIB) || true
	@echo ""
	@echo "Minimal Library:" 
	@$(SIZE) $(MINIMAL_LIB) || true
	@echo ""
	@if [ -f $(BUILD_DIR)/emmc_example$(BUILD_SUFFIX) ]; then \
		echo "Full Example:"; \
		$(SIZE) $(BUILD_DIR)/emmc_example$(BUILD_SUFFIX) || true; \
		echo ""; \
	fi
	@if [ -f $(BUILD_DIR)/emmc_minimal_example$(BUILD_SUFFIX) ]; then \
		echo "Minimal Example:"; \
		$(SIZE) $(BUILD_DIR)/emmc_minimal_example$(BUILD_SUFFIX) || true; \
	fi

# Build information
info:
	@echo "=== Build Configuration ==="
	@echo "Compiler: $(CC)"
	@echo "Build Type: $(if $(BUILD_TYPE),$(BUILD_TYPE),release)"
	@echo "LTO: $(if $(LTO),enabled,disabled)"
	@echo "Bare Metal: $(if $(BARE_METAL),enabled,disabled)"
	@echo "ARM Target: $(if $(ARM_TARGET),enabled,disabled)"
	@echo "Build Suffix: $(BUILD_SUFFIX)"
	@echo "CFLAGS: $(CFLAGS)"
	@echo "LDFLAGS: $(LDFLAGS)"
	@echo ""
	@echo "=== Source Files ==="
	@echo "Core Sources: $(CORE_SOURCES)"
	@echo "Advanced Sources: $(ADVANCED_SOURCES)"
	@echo ""
	@echo "=== Available Targets ==="
	@echo "all, full       - Build full featured library"
	@echo "minimal         - Build minimal library (core only)"
	@echo "examples        - Build example programs"
	@echo "tests           - Build test programs"
	@echo "size            - Show size comparison"
	@echo "clean           - Remove build files"
	@echo "help            - Show usage examples"

# Usage help
help:
	@echo "=== eMMC Stack Build System ==="
	@echo ""
	@echo "Basic Usage:"
	@echo "  make                    # Build full library"
	@echo "  make minimal            # Build minimal library"
	@echo "  make examples           # Build examples"
	@echo "  make size               # Compare sizes"
	@echo ""
	@echo "Build Types:"
	@echo "  make BUILD_TYPE=debug   # Debug build (-O0 -g)"
	@echo "  make BUILD_TYPE=minimal # Minimal build (-Os)"
	@echo "  make                    # Release build (-O2)"
	@echo ""
	@echo "Options:"
	@echo "  make LTO=1              # Enable Link Time Optimization"
	@echo "  make BARE_METAL=1       # Bare-metal build"
	@echo "  make ARM_TARGET=1       # ARM Cortex-A7 target"
	@echo ""
	@echo "Examples:"
	@echo "  make BUILD_TYPE=minimal LTO=1 BARE_METAL=1 ARM_TARGET=1"
	@echo "  make examples"
	@echo "  make tests"

# Clean targets
clean:
	@echo "Cleaning build files..."
	rm -rf $(BUILD_DIR)
	@echo "Clean complete"

distclean: clean
	@echo "Removing all generated files..."
	rm -f *.map
	@echo "Distclean complete"

# Build variants (shortcuts)
debug:
	$(MAKE) BUILD_TYPE=debug

release:
	$(MAKE) BUILD_TYPE=release

minimal_optimized:
	$(MAKE) BUILD_TYPE=minimal LTO=1

baremetal:
	$(MAKE) BARE_METAL=1 ARM_TARGET=1

# Installation (optional)
install: full
	@echo "Installing eMMC stack..."
	install -d /usr/local/lib
	install -d /usr/local/include/emmc
	install -m 644 $(FULL_LIB) /usr/local/lib/
	install -m 644 $(INCLUDE_DIR)/*.h /usr/local/include/emmc/
	install -m 644 $(PROTOCOL_DIR)/emmc_protocol.h /usr/local/include/emmc/
	install -m 644 $(PROTOCOL_DIR)/emmc_features.h /usr/local/include/emmc/
	@echo "Installation complete"