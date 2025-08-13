# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a generic eMMC stack firmware project designed for bare-metal environments. The project follows C99 standards and targets embedded systems without an operating system.

## Development Environment

- **Language Standard**: C99 compliant code only
- **Target Platform**: Bare-metal embedded systems
- **Architecture**: ARM-based (armv7l, aarch64)
- **Development Environment**: Termux on Android (resource-constrained)

## Code Guidelines

- All source code must be C99 compliant
- No dynamic memory allocation (use static allocation only)
- Minimal dependencies - prefer self-contained implementations
- Hardware abstraction layer (HAL) pattern for platform-specific code
- Interrupt-safe code practices for bare-metal environment

## Project Structure

```
src/
 hal/          # Hardware Abstraction Layer
 drivers/      # Low-level hardware drivers
 stack/        # eMMC protocol stack implementation
 utils/        # Utility functions and macros
 include/      # Public header files
```

## Build System

- Uses standard CMake for cross-compilation
- Target toolchain: ARM GCC cross-compiler
- Build artifacts: Binary firmware images (.bin, .elf)
- No compilation attempts in development environment due to resource constraints

## eMMC Stack Architecture

The firmware implements a layered architecture:

1. **Physical Layer**: Direct hardware register access and timing control
2. **Protocol Layer**: eMMC command/response handling and state machine
3. **Block Layer**: Sector-based read/write operations
4. **Interface Layer**: High-level API for applications

## Memory Layout

- **Code Section**: Flash memory (typically 0x08000000)
- **Data Section**: SRAM for variables and stack
- **Buffer Management**: Static buffers for eMMC data transfer
- **Interrupt Vectors**: Fixed memory locations for exception handling

## Key Components

- eMMC command set implementation (CMD0-CMD62)
- CSD/CID register parsing and management
- Boot partition and RPMB (Replay Protected Memory Block) support
- RPMB external key injection interface for HMAC authentication
- RPMB secure counter and MAC verification with external crypto backend
- Error handling and recovery mechanisms
- Power management and low-power modes

## Development Notes

- All timing-critical code should be optimized for the target MCU
- Use volatile qualifiers for hardware register access
- Implement proper cache management for DMA operations
- Consider endianness when handling multi-byte data structures

## RPMB Security Interface

- **Key Management**: External key injection through secure API interface
- **HMAC Operations**: Callback functions for external HMAC-SHA256 implementation  
- **Authentication Flow**: Support for external authentication key provisioning
- **Crypto Backend**: Abstract interface for external cryptographic functions
- **MMC Focus**: Primary emphasis on MMC protocol implementation rather than crypto internals