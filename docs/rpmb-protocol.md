# RPMB Protocol Reference

This document provides a comprehensive reference for RPMB (Replay Protected Memory Block) protocol implementation based on JESD84-B51 specification.

> **Note**: For detailed RPMB specification information including frame structure, field layouts, and constraints, see [jesd84-b51-rpmb-spec.md](jesd84-b51-rpmb-spec.md).

## Table of Contents

- [RPMB Frame Types](#rpmb-frame-types)
- [Protocol Sequences](#protocol-sequences)
- [Frame Structure](#frame-structure)
- [Security Requirements](#security-requirements)
- [Error Codes](#error-codes)
- [Implementation Guidelines](#implementation-guidelines)

## RPMB Frame Types

RPMB protocol defines 5 distinct frame types identified by the `req_resp` field:

### 1. Authentication Key Programming (0x01)
- **Purpose**: One-time programming of RPMB authentication key
- **Direction**: Host → Device
- **Response**: Result frame via Read Result operation
- **Key Fields**: `key_mac` (32-byte authentication key)

### 2. Write Counter Read (0x02)
- **Purpose**: Read current write counter value
- **Direction**: Host → Device → Host
- **Response**: Single frame with current counter
- **Key Fields**: `write_counter`, optional `nonce`

### 3. Authenticated Data Write (0x03)
- **Purpose**: Write authenticated data to RPMB partition
- **Direction**: Host → Device
- **Response**: Result frame via Read Result operation
- **Key Fields**: `data` (256 bytes), `write_counter`, `address`, `block_count`, `key_mac`

### 4. Authenticated Data Read (0x04)
- **Purpose**: Read authenticated data from RPMB partition
- **Direction**: Host → Device → Host
- **Response**: Data frames with authentication
- **Key Fields**: `address`, `block_count`, `nonce`

### 5. Result Register Read (0x05)
- **Purpose**: Read result of previous operation
- **Direction**: Host → Device → Host
- **Response**: Single frame with result code
- **Key Fields**: `result`, `write_counter`

## Protocol Sequences

### Authentication Key Programming

```
1. Host → Device: Key Programming Request
   CMD23 (SET_BLOCK_COUNT): block_count = 1
   CMD25 (WRITE_MULTIPLE_BLOCK): 
     Frame: req_resp = 0x01, key_mac = 32-byte key

2. Host → Device: Result Read Request
   CMD23 (SET_BLOCK_COUNT): block_count = 1
   CMD25 (WRITE_MULTIPLE_BLOCK):
     Frame: req_resp = 0x05

3. Host ← Device: Result Response
   CMD23 (SET_BLOCK_COUNT): block_count = 1
   CMD18 (READ_MULTIPLE_BLOCK):
     Frame: result = status code
```

### Write Counter Read

```
1. Host → Device: Counter Read Request
   CMD23 (SET_BLOCK_COUNT): block_count = 1
   CMD25 (WRITE_MULTIPLE_BLOCK):
     Frame: req_resp = 0x02, nonce = random (optional)

2. Host ← Device: Counter Response
   CMD23 (SET_BLOCK_COUNT): block_count = 1
   CMD18 (READ_MULTIPLE_BLOCK):
     Frame: write_counter = current value, MAC (if nonce provided)
```

### Authenticated Data Write

```
1. Host → Device: Data Write Request
   CMD23 (SET_BLOCK_COUNT): block_count = N, reliable_write = 1
   CMD25 (WRITE_MULTIPLE_BLOCK):
     N Frames: 
       - req_resp = 0x03
       - Same write_counter for all frames
       - data = 256 bytes per frame
       - MAC only in last frame

2. Host → Device: Result Read Request
   CMD23 (SET_BLOCK_COUNT): block_count = 1
   CMD25 (WRITE_MULTIPLE_BLOCK):
     Frame: req_resp = 0x05

3. Host ← Device: Result Response
   CMD23 (SET_BLOCK_COUNT): block_count = 1
   CMD18 (READ_MULTIPLE_BLOCK):
     Frame: result, write_counter, MAC
```

### Authenticated Data Read

```
1. Host → Device: Data Read Request
   CMD23 (SET_BLOCK_COUNT): block_count = 1
   CMD25 (WRITE_MULTIPLE_BLOCK):
     Frame: req_resp = 0x04, address, block_count, nonce

2. Host ← Device: Data Response
   CMD23 (SET_BLOCK_COUNT): block_count = N
   CMD18 (READ_MULTIPLE_BLOCK):
     N Frames:
       - data = 256 bytes per frame
       - nonce echoed back
       - MAC only in last frame
```

## Frame Structure

RPMB frames are exactly 512 bytes with the following structure:

```
Offset  Size  Field         Description                            Endian
------  ----  -----------   ------------------------------------   ------
0       196   stuff         Padding bytes (must be 0x00)          N/A
196     32    key_mac       Authentication key or MAC              N/A
228     256   data          Data payload (256 bytes)               N/A
484     16    nonce         Random nonce for replay protection     N/A
500     4     write_counter Write counter value                    Big-endian
504     2     address       Block address (0-based)                Big-endian
506     2     block_count   Number of blocks                       Big-endian
508     2     result        Result/error code                      Big-endian
510     2     req_resp      Request/response type                  Big-endian
```

### Endian Requirements (JESD84-B51 Section 6.6.22)

**All multi-byte numeric fields in RPMB frames MUST be stored in big-endian format:**

- `write_counter` (4 bytes): Big-endian format as per eMMC specification
- `address` (2 bytes): Big-endian format for block addressing
- `block_count` (2 bytes): Big-endian format for operation size
- `result` (2 bytes): Big-endian format for error codes
- `req_resp` (2 bytes): Big-endian format for operation type

**Protocol Requirements:**
- All multi-byte fields transmitted in big-endian byte order
- HMAC computation performed on big-endian frame data as transmitted
- Host implementations must handle endian conversion as appropriate for target platform

## Security Requirements

### HMAC Calculation (JESD84-B51 Section 6.6.22)

For multi-block operations, HMAC is calculated over concatenated frame data:

```
HMAC_Input = Frame₁_Data || Frame₁_Metadata || 
             Frame₂_Data || Frame₂_Metadata || 
             ... ||
             FrameN_Data || FrameN_Metadata

Where Frame_Metadata includes:
- nonce (16 bytes)
- write_counter (4 bytes)
- address (2 bytes)
- block_count (2 bytes)
- result (2 bytes)
- req_resp (2 bytes)
```

### Multi-Block Write Rules

1. **Write Counter Consistency**: All frames in a single request use the SAME write counter value
2. **MAC Placement**: MAC field is ONLY set in the LAST frame of multi-block writes
3. **HMAC Scope**: HMAC covers ALL frames' data and metadata concatenated
4. **Frame Order**: Frames are processed in sequential address order
5. **Atomicity**: Multi-block write is treated as a single atomic operation

### Replay Protection

- **Nonce**: Must be cryptographically random for read operations
- **Write Counter**: Must be monotonically increasing
- **Counter Verification**: Host must verify counter incremented correctly
- **Nonce Echo**: Device must echo nonce in read responses

## Error Codes

| Code | Name                    | Description                           |
|------|-------------------------|---------------------------------------|
| 0x00 | RPMB_RESULT_OK         | Operation successful                  |
| 0x01 | RPMB_RESULT_GENERAL    | General failure                       |
| 0x02 | RPMB_RESULT_AUTH       | Authentication failure (MAC mismatch)|
| 0x03 | RPMB_RESULT_COUNTER    | Counter failure (rollback detected)  |
| 0x04 | RPMB_RESULT_ADDRESS    | Address failure (invalid address)    |
| 0x05 | RPMB_RESULT_WRITE      | Write failure                         |
| 0x06 | RPMB_RESULT_READ       | Read failure                          |
| 0x07 | RPMB_RESULT_KEY_NOT_PROGRAMMED | Authentication key not programmed |

## Implementation Guidelines

### Mandatory Requirements

1. **Specification Compliance**: All implementations MUST follow JESD84-B51 Section 6.6.22
2. **Frame Size**: Always 512 bytes (256 data + 256 metadata)
3. **Static Allocation**: Use static buffers, no dynamic memory allocation
4. **Streaming HMAC**: Preferred method for memory efficiency
5. **Constant-Time Comparison**: Use secure MAC comparison to prevent timing attacks

### Performance Optimizations

1. **Counter Caching**: Cache write counter within transaction scope
2. **Batch Operations**: Group multiple blocks in single request when possible
3. **Stream Processing**: Use streaming HMAC to avoid large memory allocation

### Security Considerations

- **Key Protection**: Never expose RPMB key in debug output
- **Timing Attacks**: Use constant-time MAC comparison
- **Replay Protection**: Always verify nonce in read operations
- **Counter Validation**: Detect counter rollback attempts

### Error Handling Priority

1. Check RPMB result code (from device response)
2. Verify protocol compliance (frame structure)
3. Validate cryptographic authentication (MAC, nonce)
4. Confirm address/counter consistency

## References

- [jesd84-b51-rpmb-spec.md](jesd84-b51-rpmb-spec.md): Detailed RPMB specification reference
- JESD84-B51: eMMC Electrical Standard 5.1
- Section 6.6.22: RPMB Partition Protocol
- FIPS 198-1: HMAC-SHA256 Specification