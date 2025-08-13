# eMMC Generic Stack Firmware

A comprehensive eMMC stack implementation designed for bare-metal embedded systems without an operating system. This stack provides a complete layered architecture for interfacing with embedded Multi-Media Card (eMMC) storage devices.

## Features

- **Layered Architecture**: Clean separation between HAL, Driver, and Protocol layers
- **eMMC 5.1 Support**: Full compliance with eMMC specification v5.1
- **High Performance**: Supports HS200/HS400 modes with 8-bit bus width
- **Predefined Transfers**: Optimized CMD23 usage for efficient multi-block operations  
- **RPMB Support**: Replay Protected Memory Block with external crypto interface
- **Boot Partition**: Complete boot partition management and configuration
- **Cache Management**: Built-in cache control and background operations
- **Partition Support**: Full support for User, Boot1/2, RPMB, and GP partitions
- **Error Handling**: Comprehensive error detection and recovery mechanisms
- **Resource Efficient**: Designed for resource-constrained embedded systems

## Architecture

The stack follows a three-layer architecture:

```
Application Layer
       ↓
[Protocol Layer] - emmc_protocol.h/c
       ↓  
[Driver Layer] - emmc_core.h/c
       ↓
[HAL Layer] - hal_emmc.h
       ↓
Hardware Registers
```

### Hardware Abstraction Layer (HAL)
- Platform-specific hardware register access
- Clock and power management
- Interrupt handling
- DMA configuration

### Driver Layer  
- eMMC card initialization and identification
- Command/response processing
- CID/CSD/EXT_CSD parsing
- State machine management

### Protocol Layer
- High-level block I/O operations
- Partition management
- Performance optimization
- RPMB operations
- Cache and BKOPS management

## Directory Structure

```
src/
├── hal/
│   └── hal_emmc.h          # HAL interface definition
├── driver/
│   ├── emmc_core.h         # Driver interface
│   └── emmc_core.c         # Driver implementation  
├── protocol/
│   ├── emmc_protocol.h     # Core protocol interface
│   ├── emmc_protocol.c     # Core protocol implementation
│   ├── emmc_features.h     # Advanced features (auto-linked)
│   └── emmc_advanced.c     # Advanced features implementation
└── include/
    ├── emmc_types.h        # Common data types
    ├── emmc_cmd.h          # Command definitions
    └── emmc_regs.h         # Register definitions

examples/
└── basic_usage.c           # Usage examples

tests/
├── hal_stub.c             # HAL stub for testing
└── test_rpmb.c            # RPMB functionality tests

# Build system
CMakeLists.txt             # CMake configuration
Makefile                   # Make configuration
```

## Quick Start

### 1. Platform Integration

Implement the HAL functions for your specific platform:

```c
/* Implement these functions in your platform-specific code */
emmc_result_t hal_emmc_init(const hal_emmc_config_t *config);
u32 hal_emmc_set_clock(u32 frequency);
emmc_result_t hal_emmc_send_command(const hal_emmc_cmd_t *cmd, const hal_emmc_data_t *data);
/* ... other HAL functions */
```

### 2. Basic Usage

```c
#include "src/protocol/emmc_protocol.h"

/* Configure the stack */
emmc_protocol_config_t config = {
    .driver_config = {
        .hal_config = {
            .base_address = 0x40000000,    /* Your eMMC controller base */
            .max_clock_freq = 200000000,   /* 200 MHz */
            .dma_enabled = true,
            .max_bus_width = 8
        },
        .init_timeout_ms = 1000,
        .enable_cache = true
    },
    .auto_optimize = true,
    .enable_advanced_features = true
};

/* Initialize */
emmc_protocol_init(&config);
emmc_initialize();

/* Read/Write operations */
u8 buffer[4096];
emmc_read_sectors(0x1000, 8, buffer);   /* Read 8 sectors */
emmc_write_sectors(0x1000, 8, buffer);  /* Write 8 sectors */
```

### 3. RPMB Usage

For RPMB (secure storage), provide external crypto functions:

```c
emmc_rpmb_crypto_interface_t crypto = {
    .get_key = your_key_retrieval_func,
    .generate_nonce = your_nonce_generation_func,
    .compute_hmac = your_hmac_sha256_func,
    .verify_hmac = your_hmac_verify_func
};

emmc_rpmb_init(&crypto);

/* Program authentication key (one-time operation) */
u8 auth_key[32] = { /* your 256-bit authentication key */ };
emmc_rpmb_program_key(auth_key);

/* Write secure data (uses key from crypto interface) */
u8 secure_data[256] = { /* your data */ };
emmc_rpmb_write_data(0, secure_data, 1);  /* address 0, 1 block */

/* Read secure data (automatically authenticated) */
u8 read_buffer[256];
emmc_rpmb_read_data(0, read_buffer, 1);   /* address 0, 1 block */

/* Multi-block operations (up to 32 blocks) */
u8 large_data[32 * 256];  /* 32 blocks worth of data */
emmc_rpmb_write_multi(0, large_data, 32);
emmc_rpmb_read_multi(0, large_data, 32);
```

## Advanced Features

### Performance Optimization

The stack automatically optimizes performance by:
- Selecting optimal bus width (8-bit preferred)
- Choosing best timing mode (HS200/HS400 if supported)  
- Using CMD23 predefined block count for large transfers
- Enabling cache when available

### Partition Management

```c
/* Switch partitions */
emmc_select_partition(EMMC_PART_BOOT1);
emmc_write_sectors(0, 1, boot_code);

emmc_select_partition(EMMC_PART_USER);
emmc_read_sectors(0x1000, 64, data_buffer);
```

### Error Handling

All functions return `emmc_result_t` status codes:
- `EMMC_OK`: Success
- `EMMC_TIMEOUT`: Operation timeout
- `EMMC_CRC_ERROR`: CRC verification failed
- `EMMC_BUSY`: Device busy
- `EMMC_NOT_READY`: Device not initialized

## Memory Requirements

- **Code (Minimal)**: ~12KB (core functions only)
- **Code (Full)**: ~22KB (all features included)
- **RAM**: ~2KB static data + buffers  
- **Stack**: ~1KB maximum call depth

### Dead Code Elimination

The stack uses function-level linking to automatically remove unused features:

```bash
# Minimal build (basic read/write only)
make minimal          # ~12KB code size

# Full build (all features)
make full            # ~22KB code size

# Automatic optimization - only used functions are linked
make examples        # Links only the functions actually called
```

## Development Guidelines

### C99 Compliance
- All code follows C99 standard
- No dynamic memory allocation
- Static buffer management only

### Bare-Metal Considerations  
- No OS dependencies
- Interrupt-safe design
- Hardware register volatile access
- Proper cache management

### Security
- RPMB uses external crypto interface
- No hardcoded keys or secrets
- Secure key injection support

## Building

### Using Make
```bash
# Basic builds
make                    # Full featured build (release)
make minimal           # Core functions only
make debug             # Debug build with symbols

# Optimized builds  
make LTO=1             # Enable Link Time Optimization
make BARE_METAL=1      # Bare-metal optimized build
make ARM_TARGET=1      # ARM Cortex-A7 optimized

# Combined optimizations
make BUILD_TYPE=minimal LTO=1 BARE_METAL=1 ARM_TARGET=1

# Examples and tests
make examples          # Build example programs
make tests            # Build test programs
make size             # Compare build sizes
```

### Using CMake
```bash
mkdir build && cd build

# Basic build
cmake .. && make

# Minimal build  
cmake -DCMAKE_BUILD_TYPE=MinSizeRel .. && make emmc_stack_minimal

# With examples and tests
cmake -DEMMC_BUILD_EXAMPLES=ON -DEMMC_BUILD_TESTS=ON .. && make
```

### Integration Example
```cmake
# Add to your project
add_subdirectory(emmc-stack)
target_link_libraries(your_target emmc_stack)

# Or minimal version
target_link_libraries(your_target emmc_stack_minimal)
```

## Testing

A HAL stub implementation is provided for testing:

```c
#include "tests/hal_stub.c"  /* Use stub HAL for testing */
#include "examples/basic_usage.c"

/* Run tests without hardware */
```

## Contributing

1. Follow C99 coding standards
2. Maintain layer separation
3. Add comprehensive error handling  
4. Include usage examples
5. Test with HAL stub first

## License

This project is designed for embedded systems development. Please ensure compliance with your project's licensing requirements.

## Platform Support

Tested and verified on:
- ARM Cortex-M series
- ARM Cortex-A series  
- RISC-V embedded cores

The HAL abstraction allows easy porting to other architectures.