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
- **Dead Code Elimination Priority**: All conditional code must prioritize compiler dead code elimination for memory optimization

## Memory Optimization Guidelines

- **Conditional Compilation**: Use `if (STATIC_MACRO)` instead of `#if` for dead code elimination
- **Constant Propagation**: Leverage compiler optimization for static configuration
- **Zero Runtime Cost**: Static configurations should have zero runtime overhead
- **Memory Footprint**: Minimize binary size through compile-time optimization

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

## RPMB Specification Compliance

**CRITICAL**: All RPMB implementations MUST strictly follow JESD84-B51 specification and reference docs/rpmb-protocol.md for complete implementation guidance.

### Core RPMB Protocol Requirements

- **Specification Reference**: JESD84-B51 (eMMC Electrical Standard 5.1)
- **RPMB Frame Size**: Fixed 512 bytes (256 bytes data + 256 bytes metadata)
- **Authentication**: HMAC-SHA256 based message authentication
- **Write Counter**: Monotonic counter prevents replay attacks
- **Block Addressing**: 0-based addressing within RPMB partition

### Multi-Block Operation Rules

1. **Write Counter Consistency**: All frames in single request use SAME write counter
2. **HMAC Calculation**: Compute HMAC over ALL frames concatenated data
3. **MAC Placement**: MAC field ONLY in the LAST frame of multi-block write
4. **Frame Order**: Frames processed in sequential address order
5. **Atomicity**: Multi-block write treated as single atomic operation

### Authentication Data Layout (per JESD84-B51 Section 6.6.22)

```
HMAC Input = Frame₁_Data || Frame₁_Metadata || Frame₂_Data || Frame₂_Metadata || ... || FrameN_Data || FrameN_Metadata

Where each Frame_Metadata includes:
- Nonce (16 bytes)
- Write Counter (4 bytes) 
- Address (2 bytes)
- Block Count (2 bytes)
- Result (2 bytes)
- Request/Response (2 bytes)
```

## RPMB Implementation Guidelines

### Mandatory Implementation Rules

1. **Always validate against JESD84-B51**: Before implementing ANY RPMB feature, reference section 6.6.22 and docs/rpmb-protocol.md
2. **Write Counter Management**:
   - Single request = Single write counter value for ALL frames
   - Counter increments ONLY after successful multi-frame write completion
   - Never assume counter values - always query from device
   
3. **HMAC Computation Strategy**:
   - **Preferred**: Streaming HMAC (init → update per frame → final)
   - **Fallback**: Concatenated buffer HMAC (all frames → single compute)
   - **Never**: Individual frame HMAC (spec violation)

4. **Error Handling Priority**:
   ```c
   // Check order for RPMB errors:
   1. RPMB result code (from device response)
   2. Protocol compliance (frame structure)
   3. Cryptographic validation (MAC, nonce)
   4. Address/counter consistency
   ```

### Security Considerations

- **Replay Protection**: Always verify nonce in read operations
- **MAC Verification**: Fail immediately on MAC mismatch - never retry
- **Counter Validation**: Detect counter rollback attempts
- **Key Management**: Never expose RPMB key in debug output
- **Timing Attacks**: Use constant-time MAC comparison

### Performance Optimization Guidelines

- **Batch Operations**: Group multiple blocks in single request when possible
- **Stream Processing**: Use streaming HMAC to avoid large memory allocation
- **Static Buffers**: Pre-allocate frame buffers to avoid runtime allocation
- **Counter Caching**: Cache write counter within single transaction scope

### Code Review Checklist

Before committing any RPMB code changes:
- [ ] Verified against JESD84-B51 specification section 6.6.22 and docs/rpmb-protocol.md
- [ ] Multi-block writes use single counter value
- [ ] HMAC calculated over concatenated frame data
- [ ] MAC placed only in last frame
- [ ] Proper error code mapping implemented
- [ ] No cryptographic material in logs/debug output

## RPMB Testing Checklist

### Specification Compliance Testing

**Single Block Operations**:
- [ ] Write single block with correct HMAC placement
- [ ] Read single block with nonce verification  
- [ ] Write counter increments correctly (+1)
- [ ] Authentication failure detection

**Multi-Block Operations**:
- [ ] All frames use same write counter value
- [ ] HMAC computed over ALL frame data concatenated
- [ ] MAC present ONLY in last frame (others zero)
- [ ] Sequential address processing verified
- [ ] Counter increments by 1 (not frame count)

**Error Condition Testing**:
- [ ] Invalid MAC rejection (EMMC_MAC_ERROR)
- [ ] Counter mismatch detection (EMMC_COUNTER_ERROR)
- [ ] Authentication failure handling (EMMC_AUTH_ERROR)
- [ ] Protocol violation detection (EMMC_PROTOCOL_ERROR)
- [ ] Address validation (EMMC_ADDRESS_ERROR)
- [ ] Nonce replay detection (EMMC_NONCE_ERROR)

### Security Validation

**Cryptographic Verification**:
- [ ] HMAC-SHA256 calculation correctness
- [ ] Key isolation (no key exposure in debugging)
- [ ] Constant-time MAC comparison
- [ ] Secure nonce generation (crypto-grade randomness)

**Replay Attack Protection**:
- [ ] Write counter monotonic increase
- [ ] Nonce uniqueness verification
- [ ] Counter rollback detection
- [ ] Timestamp validation (if implemented)

### Performance Verification

**Optimization Effectiveness**:
- [ ] Streaming HMAC vs fallback buffer method
- [ ] Single counter query per multi-block operation
- [ ] Static buffer reuse (no dynamic allocation)
- [ ] Frame batching efficiency measurement

**Benchmark Requirements**:
- [ ] Single vs multi-block write performance comparison
- [ ] HMAC computation time measurement
- [ ] Memory usage analysis (static allocation only)
- [ ] Real-world performance vs specification limits

### Integration Testing

**Cross-Platform Compatibility**:
- [ ] ARM Cortex-M series compatibility
- [ ] AArch64 platform testing
- [ ] Endianness handling verification
- [ ] C99 compliance validation

**External Interface Testing**:
- [ ] Crypto interface callback validation
- [ ] Key management interface testing
- [ ] Error propagation through stack layers
- [ ] HAL integration verification

### Production Readiness Checklist

- [ ] All JESD84-B51 requirements implemented per docs/rpmb-protocol.md
- [ ] Zero dynamic memory allocation
- [ ] Interrupt-safe operation confirmed
- [ ] Error recovery mechanisms tested
- [ ] Documentation updated with spec references
- [ ] Code review completed by second developer
- [ ] Security audit passed (if required)
- [ ] Performance benchmarks meet requirements
