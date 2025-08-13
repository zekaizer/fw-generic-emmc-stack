# eMMC Generic Stack CMake Configuration
cmake_minimum_required(VERSION 3.16)

project(emmc_stack 
    VERSION 1.0.0
    DESCRIPTION "Generic eMMC stack firmware for bare-metal systems"
    LANGUAGES C
)

# C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler optimization flags for different build types
set(CMAKE_C_FLAGS_DEBUG "-O0 -g3 -DDEBUG")
set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG")
set(CMAKE_C_FLAGS_MINSIZEREL "-Os -DNDEBUG")

# Function-level linking for dead code elimination
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffunction-sections -fdata-sections")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-common")

# Linker flags for garbage collection of unused sections
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--print-gc-sections")

# Optional: Link Time Optimization (LTO)
option(EMMC_ENABLE_LTO "Enable Link Time Optimization" OFF)
if(EMMC_ENABLE_LTO)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -flto")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto")
endif()

# Source files
set(EMMC_CORE_SOURCES
    src/driver/emmc_core.c
    src/protocol/emmc_protocol.c
)

# Advanced features (always compiled, but linked only if used)
set(EMMC_ADVANCED_SOURCES
    src/protocol/emmc_advanced.c
)

# Create main eMMC stack library
add_library(emmc_stack STATIC 
    ${EMMC_CORE_SOURCES}
    ${EMMC_ADVANCED_SOURCES}
)

# Include directories
target_include_directories(emmc_stack PUBLIC 
    src/include
    src/hal
    src/driver
    src/protocol
)

# Compiler definitions
target_compile_definitions(emmc_stack PRIVATE
    EMMC_STACK_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    EMMC_STACK_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    EMMC_STACK_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)

# Platform-specific options
option(EMMC_BARE_METAL "Build for bare-metal (no OS)" ON)
option(EMMC_ARM_TARGET "Target ARM architecture" ON)

# Preset selection
set(EMMC_PRESET "dynamic" CACHE STRING "Choose eMMC preset: tiny, embedded, secure, performance, bootloader, recovery, or dynamic")
set_property(CACHE EMMC_PRESET PROPERTY STRINGS "tiny" "embedded" "secure" "performance" "bootloader" "recovery" "dynamic")

# Apply preset-specific compiler definitions
if(EMMC_PRESET STREQUAL "tiny")
    target_compile_definitions(emmc_stack PRIVATE EMMC_PRESET_TINY=1)
    message(STATUS "Using TINY preset - Estimated size: 6-8KB")
elseif(EMMC_PRESET STREQUAL "embedded")
    target_compile_definitions(emmc_stack PRIVATE EMMC_PRESET_EMBEDDED=1)
    message(STATUS "Using EMBEDDED preset - Estimated size: 12-14KB")
elseif(EMMC_PRESET STREQUAL "secure")
    target_compile_definitions(emmc_stack PRIVATE EMMC_PRESET_SECURE=1)
    message(STATUS "Using SECURE preset - Estimated size: 16-18KB")
elseif(EMMC_PRESET STREQUAL "performance")
    target_compile_definitions(emmc_stack PRIVATE EMMC_PRESET_PERFORMANCE=1)
    message(STATUS "Using PERFORMANCE preset - Estimated size: 20-22KB")
elseif(EMMC_PRESET STREQUAL "bootloader")
    target_compile_definitions(emmc_stack PRIVATE EMMC_PRESET_BOOTLOADER=1)
    message(STATUS "Using BOOTLOADER preset - Estimated size: 10-12KB")
elseif(EMMC_PRESET STREQUAL "recovery")
    target_compile_definitions(emmc_stack PRIVATE EMMC_PRESET_RECOVERY=1)
    message(STATUS "Using RECOVERY preset - Estimated size: 16-18KB")
else()
    message(STATUS "Using DYNAMIC preset - Estimated size: 20-25KB")
endif()

if(EMMC_BARE_METAL)
    target_compile_definitions(emmc_stack PRIVATE EMMC_BARE_METAL=1)
    # Remove standard library dependencies
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -nostdlib -ffreestanding")
endif()

if(EMMC_ARM_TARGET)
    target_compile_definitions(emmc_stack PRIVATE EMMC_ARM_TARGET=1)
    # ARM-specific optimizations
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mcpu=cortex-a7 -mthumb")
endif()

# Test executables (optional)
option(EMMC_BUILD_TESTS "Build test executables" OFF)
if(EMMC_BUILD_TESTS)
    add_executable(emmc_test_basic
        examples/basic_usage.c
        tests/hal_stub.c
    )
    target_link_libraries(emmc_test_basic emmc_stack)
    
    add_executable(emmc_test_rpmb
        tests/test_rpmb.c
        tests/hal_stub.c
    )
    target_link_libraries(emmc_test_rpmb emmc_stack)
endif()

# Example executables
option(EMMC_BUILD_EXAMPLES "Build example executables" OFF)
if(EMMC_BUILD_EXAMPLES)
    add_executable(emmc_example
        examples/basic_usage.c
        tests/hal_stub.c  # Use HAL stub for examples
    )
    target_link_libraries(emmc_example emmc_stack)
    
    # Show which functions are being linked
    target_link_options(emmc_example PRIVATE
        -Wl,-Map=emmc_example.map
        -Wl,--print-gc-sections
    )
endif()

# Install configuration
install(TARGETS emmc_stack
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY src/include/ DESTINATION include/emmc)
install(FILES src/protocol/emmc_protocol.h DESTINATION include/emmc)
install(FILES src/protocol/emmc_features.h DESTINATION include/emmc)

# Build configuration summary
message(STATUS "eMMC Stack Build Configuration:")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C Standard: C${CMAKE_C_STANDARD}")
message(STATUS "  Preset: ${EMMC_PRESET}")
message(STATUS "  LTO: ${EMMC_ENABLE_LTO}")
message(STATUS "  Bare Metal: ${EMMC_BARE_METAL}")
message(STATUS "  ARM Target: ${EMMC_ARM_TARGET}")
message(STATUS "  Tests: ${EMMC_BUILD_TESTS}")
message(STATUS "  Examples: ${EMMC_BUILD_EXAMPLES}")

# Size optimization report (if available)
if(CMAKE_SIZE)
    add_custom_target(size
        COMMAND ${CMAKE_SIZE} $<TARGET_FILE:emmc_stack>
        DEPENDS emmc_stack
        COMMENT "Displaying size information"
    )
endif()

# Create preset-specific build variants  
add_library(emmc_stack_tiny STATIC
    src/driver/emmc_core.c
    src/protocol/emmc_protocol.c
    # Note: emmc_advanced.c is NOT included
)

target_include_directories(emmc_stack_tiny PUBLIC 
    src/include
    src/hal  
    src/driver
    src/protocol
)

target_compile_definitions(emmc_stack_tiny PRIVATE
    EMMC_PRESET_TINY=1
    EMMC_MINIMAL_BUILD=1
)

# Also create a minimal build variant for backward compatibility
add_library(emmc_stack_minimal STATIC
    src/driver/emmc_core.c
    src/protocol/emmc_protocol.c
    # Note: emmc_advanced.c is NOT included
)

target_include_directories(emmc_stack_minimal PUBLIC 
    src/include
    src/hal  
    src/driver
    src/protocol
)

target_compile_definitions(emmc_stack_minimal PRIVATE
    EMMC_MINIMAL_BUILD=1
)

# Minimal build example
if(EMMC_BUILD_EXAMPLES)
    add_executable(emmc_minimal_example
        examples/basic_usage.c
        tests/hal_stub.c
    )
    target_link_libraries(emmc_minimal_example emmc_stack_minimal)
    
    target_link_options(emmc_minimal_example PRIVATE
        -Wl,-Map=emmc_minimal_example.map
    )
endif()

message(STATUS "")
message(STATUS "Build Targets:")
message(STATUS "  emmc_stack         - Full featured library")
message(STATUS "  emmc_stack_minimal - Core functions only")
message(STATUS "  emmc_stack_tiny    - Tiny preset library") 
if(EMMC_BUILD_EXAMPLES)
    message(STATUS "  emmc_example       - Full featured example")
    message(STATUS "  emmc_minimal_example - Minimal example")
endif()
if(EMMC_BUILD_TESTS)
    message(STATUS "  emmc_test_basic    - Basic functionality test")
    message(STATUS "  emmc_test_rpmb     - RPMB functionality test")
endif()

message(STATUS "")
message(STATUS "Usage Examples:")
message(STATUS "  cmake .. -DEMMC_PRESET=tiny")
message(STATUS "  cmake .. -DEMMC_PRESET=embedded -DEMMC_ARM_TARGET=ON") 
message(STATUS "  cmake .. -DEMMC_PRESET=secure -DEMMC_BUILD_EXAMPLES=ON")
message(STATUS "  cmake .. -DEMMC_PRESET=performance -DEMMC_ENABLE_LTO=ON")