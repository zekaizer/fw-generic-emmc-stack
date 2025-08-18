# JESD84-B51 RPMB Specification Reference

This document contains detailed information from JEDEC Standard JESD84-B51 "Embedded Multi-media card (e*MMC), Electrical Standard (5.1)" Section 6.6.22 regarding the Replay Protected Memory Block (RPMB) partition.

**Document Version**: JESD84-B51 (February 2015)  
**Section Reference**: 6.6.22 RPMB Partition  
**Page Reference**: 166-176

## Table of Contents

- [Overview](#overview)
- [RPMB Partition Properties](#rpmb-partition-properties)
- [RPMB Data Frame Structure](#rpmb-data-frame-structure)
- [Frame Field Descriptions](#frame-field-descriptions)
- [Request/Response Types](#requestresponse-types)
- [Operation Result Codes](#operation-result-codes)
- [Protocol Sequences](#protocol-sequences)
- [Address and Size Constraints](#address-and-size-constraints)
- [Alignment Requirements](#alignment-requirements)
- [Multi-Block Operations](#multi-block-operations)
- [Security Requirements](#security-requirements)
- [RPMB Commands](#rpmb-commands)

## Overview

The Replay Protected Memory Block (RPMB) is provided as a means for a system to store data to the specific memory area in an authenticated and replay protected manner. The RPMB can only be read and written via successfully authenticated read and write accesses. The data may be overwritten by the host but can never be erased.

### Key Characteristics (per JESD84-B51 Section 6.6.22)

- **Frame Size**: 512 bytes total per RPMB frame (half sector)
- **Data Size**: 256 bytes of user data per frame  
- **Authentication**: HMAC-SHA256 based with 32-byte MAC
- **Replay Protection**: Monotonic write counter and nonce
- **Maximum Size**: 16MB maximum capacity (RPMB_SIZE_MULT × 128KB)
- **Addressing**: Half sector addressing (256-byte data blocks)
- **Access Method**: Only through RPMB-specific commands
- **Partition Type**: Special partition (not user accessible)

## RPMB Partition Properties

### Partition Configuration (from EXT_CSD)

The RPMB partition size is configured through the EXT_CSD register:

- **RPMB_SIZE_MULT** (EXT_CSD[168]): Defines RPMB data area size
- **Size Calculation**: RPMB size = RPMB_SIZE_MULT × 128 KB
- **Maximum RPMB_SIZE_MULT**: 128 (resulting in 16MB maximum)
- **Minimum Unit**: 128KB (when RPMB_SIZE_MULT = 1)

### Partition Access Rules

1. **Authentication Required**: All access requires prior authentication key programming
2. **No Erase Operations**: Data can be overwritten but never erased
3. **Atomic Operations**: Single frame write operations are guaranteed atomic
4. **Replay Protection**: Write counter prevents replay attacks
5. **MAC Verification**: All operations require MAC verification

## RPMB Data Frame Structure

Each RPMB frame is exactly 512 bytes (one half sector) as defined in JESD84-B51 Table 17. The frame format supports both request and response operations with fields arranged in MSB (Most Significant Byte) first order.

### Frame Layout (JESD84-B51 Table 17)

```
Byte Range   Size   Field           Description                              Endian
-----------  ----   -------------   --------------------------------------   -----------
[511:316]    196    Stuff Bytes     Reserved, filled with 0x00             N/A
[315:284]    32     Key/MAC         Authentication Key or Message MAC       Big-endian
[283:28]     256    Data            User data (256 bytes)                   Big-endian
[27:12]      16     Nonce           Random number for replay protection     Big-endian
[11:8]       4      Write Counter   Monotonic counter value                 Big-endian
[7:6]        2      Address         Half sector address (0-based)           Big-endian
[5:4]        2      Block Count     Number of half sectors                   Big-endian
[3:2]        2      Result          Operation result code                   Big-endian
[1:0]        2      Req/Resp        Request/Response type                   Big-endian
```

### Byte-Level Frame Layout

```
Byte Offset: 0    196  228                            484  500 504 506 508 510 512
            |<-196->|<32>|<---------- 256 ----------->|<16>|<4>|<2>|<2>|<2>|<2>|
            | Stuff |MAC |           Data             |Nonce|WC |Addr|BC |Res|RR |
```

### Frame Size Verification

- **Total Frame Size**: 512 bytes (as per JESD84-B51 specification)
- **Half Sector**: RPMB uses half sector addressing (256 bytes user data per frame)
- **Alignment**: Frame aligns with eMMC physical sector boundaries

## Frame Field Descriptions

### Stuff Bytes (196 bytes, byte range [511:316])
- **Purpose**: Reserved space for alignment and future extensions
- **Content**: Must be filled with 0x00 (all zeros)
- **Size**: 196 bytes (largest field in frame)
- **Usage**: Not used in current specification but reserved for compatibility

### Key/MAC (32 bytes, byte range [315:284])
- **Authentication Key Programming**: Contains the 256-bit (32-byte) authentication key
- **Data Operations**: Contains HMAC-SHA256 MAC for frame authentication
- **MAC Placement Rules**:
  - Single frame: MAC in the frame
  - Multi-frame write: MAC only in the LAST frame (others contain 0x00)
  - Multi-frame read response: MAC only in the LAST frame
- **Endianness**: Big-endian format for multi-byte MAC values

### Data (256 bytes, byte range [283:28])
- **User Payload**: Actual data storage space (256 bytes per frame)
- **Content Type**: Raw binary data, no format restrictions
- **Endianness**: Data content endianness is application-defined
- **Address Mapping**: Each 256-byte block corresponds to one half-sector address
- **Usage**: This is the primary storage accessed by applications

### Nonce (16 bytes, byte range [27:12])
- **Purpose**: Random number for replay attack prevention  
- **Generation**: Host generates cryptographically random 128-bit value
- **Usage Rules**:
  - Required for read operations
  - Optional for write counter read
  - Not used in write operations
- **Echo Behavior**: Device copies nonce from request to response
- **Format**: Big-endian 128-bit number

### Write Counter (4 bytes, byte range [11:8])
- **Purpose**: Monotonic counter preventing replay attacks
- **Size**: 32-bit unsigned integer (range: 0 to 4,294,967,295)
- **Format**: Big-endian
- **Behavior**:
  - Incremented by 1 after each successful authenticated write
  - Cannot be reset or decremented
  - Shared across all RPMB operations on the device
  - Read-only for host (device manages incrementing)

### Address (2 bytes, byte range [7:6])
- **Purpose**: Specifies target half-sector address for operation
- **Size**: 16-bit unsigned integer
- **Format**: Big-endian
- **Range**: 0 to (RPMB_SIZE / 256) - 1
- **Addressing Unit**: Half-sectors (256-byte blocks)
- **Maximum**: 0xFFFF (65535) but limited by actual RPMB partition size

### Block Count (2 bytes, byte range [5:4])  
- **Purpose**: Number of half-sectors (256-byte blocks) to transfer
- **Size**: 16-bit unsigned integer
- **Format**: Big-endian
- **Range**: 1 to implementation-defined maximum
- **Multi-block Limit**: Typically limited to 32 blocks in practice
- **Usage**: Must be > 0 for data read/write operations

### Result (2 bytes, byte range [3:2])
- **Purpose**: Operation result/error code from device
- **Size**: 16-bit unsigned integer  
- **Format**: Big-endian
- **Usage**: Set by device in response frames
- **Values**: See [Operation Result Codes](#operation-result-codes) section

### Req/Resp (2 bytes, byte range [1:0])
- **Purpose**: Identifies operation type and direction
- **Size**: 16-bit unsigned integer
- **Format**: Big-endian  
- **Request Types**: 0x0001-0x0007 (host to device)
- **Response Types**: 0x0100-0x0700 (device to host)
- **Usage**: Determines frame interpretation and processing

## Request/Response Types

As defined in JESD84-B51 Section 6.6.22, RPMB operations use specific request and response type codes to identify the operation being performed.

### Request Message Types (Host → Device)

| Value  | Name                                    | Description                                | Frame Usage        |
|--------|----------------------------------------|--------------------------------------------|-------------------|
| 0x0001 | Authentication Key Programming Request | One-time programming of authentication key | Single frame      |
| 0x0002 | Write Counter Read Request             | Read current write counter value           | Single frame      |
| 0x0003 | Authenticated Data Write Request       | Write data with authentication             | 1-N frames        |
| 0x0004 | Authenticated Data Read Request        | Read data with authentication              | Single frame      |
| 0x0005 | Result Read Request                    | Read result of previous operation          | Single frame      |
| 0x0006 | Device Configuration Write Request     | Write authenticated device configuration   | Single frame      |
| 0x0007 | Device Configuration Read Request      | Read authenticated device configuration    | Single frame      |

### Response Message Types (Device → Host)

| Value  | Name                                     | Description                                | Frame Usage        |
|--------|------------------------------------------|--------------------------------------------|-------------------|
| 0x0100 | Authentication Key Programming Response  | Response to key programming                | Via result read   |
| 0x0200 | Write Counter Read Response              | Response with current counter value        | Single frame      |
| 0x0300 | Authenticated Data Write Response        | Response to data write operation           | Via result read   |
| 0x0400 | Authenticated Data Read Response         | Response with requested data               | 1-N frames        |
| 0x0600 | Device Configuration Write Response      | Response to configuration write            | Via result read   |
| 0x0700 | Device Configuration Read Response       | Response with configuration data           | Single frame      |

### Operation Flow Patterns

1. **Immediate Response Operations** (types 0x0002, 0x0004, 0x0007):
   - Host sends request → Device immediately responds

2. **Result Read Operations** (types 0x0001, 0x0003, 0x0006):
   - Host sends request → Host sends result read request → Device responds

## Operation Result Codes

The Result field contains status information as defined in JESD84-B51 Section 6.6.22. These codes indicate the outcome of RPMB operations.

| Code   | Name                       | Description                                           | Typical Causes                    |
|--------|---------------------------|-------------------------------------------------------|-----------------------------------|
| 0x0000 | Operation OK              | Operation completed successfully                      | Normal successful operation       |
| 0x0001 | General Failure           | Unspecified failure                                   | Hardware error, invalid state     |
| 0x0002 | Authentication Failure    | MAC comparison failed or MAC calculation failed       | Wrong key, corrupted MAC          |
| 0x0003 | Counter Failure           | Counter mismatch or counter increment failure         | Replay attack, counter overflow   |
| 0x0004 | Address Failure           | Address out of range or wrong address alignment      | Invalid address, size overflow    |
| 0x0005 | Write Failure             | Data/counter/result write failure                     | Storage error, write protection   |
| 0x0006 | Read Failure              | Data/counter/result read failure                      | Storage error, read failure       |
| 0x0007 | Key Not Programmed        | Authentication key not yet programmed                 | Key programming required first    |

### Result Code Usage Rules

1. **Success Path**: Only 0x0000 indicates successful completion
2. **Error Handling**: All non-zero codes indicate failure conditions  
3. **Authentication Errors**: Code 0x0002 requires key re-verification
4. **Counter Errors**: Code 0x0003 may indicate security attack
5. **Address Errors**: Code 0x0004 requires parameter validation

## Protocol Sequences

This section describes the detailed command sequences for RPMB operations as specified in JESD84-B51 Section 6.6.22.

### 1. Authentication Key Programming Sequence

```
Step 1: Host → Device (Key Programming Request)
  CMD23: SET_BLOCK_COUNT = 1, Reliable Write = 1  
  CMD25: WRITE_MULTIPLE_BLOCK, argument = 0
  Data: RPMB frame with:
    - req_resp = 0x0001
    - key_mac = 32-byte authentication key
    - All other fields = 0x00

Step 2: Host → Device (Result Read Request)  
  CMD23: SET_BLOCK_COUNT = 1
  CMD25: WRITE_MULTIPLE_BLOCK, argument = 0
  Data: RPMB frame with:
    - req_resp = 0x0005
    - All other fields = 0x00

Step 3: Host ← Device (Result Response)
  CMD23: SET_BLOCK_COUNT = 1  
  CMD18: READ_MULTIPLE_BLOCK, argument = 0
  Data: RPMB frame with:
    - req_resp = 0x0100
    - result = operation status
    - All other fields = 0x00
```

### 2. Write Counter Read Sequence

```
Step 1: Host → Device (Counter Read Request)
  CMD23: SET_BLOCK_COUNT = 1
  CMD25: WRITE_MULTIPLE_BLOCK, argument = 0  
  Data: RPMB frame with:
    - req_resp = 0x0002
    - nonce = random 16-byte value (optional)
    - All other fields = 0x00

Step 2: Host ← Device (Counter Response)
  CMD23: SET_BLOCK_COUNT = 1
  CMD18: READ_MULTIPLE_BLOCK, argument = 0
  Data: RPMB frame with:
    - req_resp = 0x0200
    - write_counter = current counter value
    - nonce = echoed from request (if provided)
    - key_mac = MAC (if nonce was provided)
    - result = 0x0000 (success)
```

### 3. Authenticated Data Write Sequence

```
Step 1: Host → Device (Data Write Request)
  CMD23: SET_BLOCK_COUNT = N, Reliable Write = 1
  CMD25: WRITE_MULTIPLE_BLOCK, argument = 0
  Data: N RPMB frames with:
    - req_resp = 0x0003 (all frames)
    - write_counter = same value for all frames
    - address = starting address (frame 0), incremented for subsequent frames
    - block_count = N (total number of frames)
    - data = 256 bytes user data per frame
    - key_mac = MAC only in LAST frame, 0x00 in others

Step 2: Host → Device (Result Read Request)
  CMD23: SET_BLOCK_COUNT = 1
  CMD25: WRITE_MULTIPLE_BLOCK, argument = 0
  Data: RPMB frame with:
    - req_resp = 0x0005
    - All other fields = 0x00

Step 3: Host ← Device (Write Result Response)
  CMD23: SET_BLOCK_COUNT = 1
  CMD18: READ_MULTIPLE_BLOCK, argument = 0  
  Data: RPMB frame with:
    - req_resp = 0x0300
    - write_counter = incremented value
    - key_mac = MAC authenticating the response
    - result = operation status
```

### 4. Authenticated Data Read Sequence

```
Step 1: Host → Device (Data Read Request)
  CMD23: SET_BLOCK_COUNT = 1
  CMD25: WRITE_MULTIPLE_BLOCK, argument = 0
  Data: RPMB frame with:
    - req_resp = 0x0004
    - address = starting address
    - block_count = number of frames to read
    - nonce = random 16-byte value
    - All other fields = 0x00

Step 2: Host ← Device (Data Response)
  CMD23: SET_BLOCK_COUNT = N  
  CMD18: READ_MULTIPLE_BLOCK, argument = 0
  Data: N RPMB frames with:
    - req_resp = 0x0400 (all frames)
    - address = frame address (incremented per frame)
    - block_count = N (total frames)
    - data = 256 bytes user data per frame
    - nonce = echoed from request (all frames)
    - key_mac = MAC only in LAST frame, 0x00 in others
    - result = 0x0000 (success)
```

## RPMB Commands

RPMB operations use standard eMMC commands with specific argument values and data patterns.

### Command Usage

| Command | Usage in RPMB | Description |
|---------|---------------|-------------|
| CMD23 | SET_BLOCK_COUNT | Sets number of frames, enables reliable write |
| CMD25 | WRITE_MULTIPLE_BLOCK | Sends request frames to device |
| CMD18 | READ_MULTIPLE_BLOCK | Reads response frames from device |

### CMD23 Arguments

- **Block Count**: Number of 512-byte RPMB frames
- **Reliable Write Flag**: Bit 31 set for write operations requiring atomicity
- **RPMB Context**: CMD argument = 0 for RPMB partition access

### Reliable Write Requirement

Write operations (key programming, data write) must use CMD23 with reliable write flag:
```
CMD23_ARG = block_count | (1 << 31)  // Set bit 31 for reliable write
```

## Address and Size Constraints

Based on JESD84-B51 Section 6.6.22 and EXT_CSD register definitions.

### Address Field Constraints
- **Field Size**: 2 bytes (16-bit) 
- **Format**: Big-endian unsigned integer
- **Range**: 0 to 0xFFFF (65535 theoretical maximum)
- **Unit**: Half-sectors (256-byte data blocks)
- **Addressing**: 0-based indexing (first block = address 0)
- **Practical Limit**: Constrained by actual RPMB partition size

### Block Count Constraints  
- **Field Size**: 2 bytes (16-bit)
- **Format**: Big-endian unsigned integer
- **Range**: 1 to implementation-defined maximum
- **Unit**: Number of half-sectors (256-byte data blocks)
- **Multi-block Limit**: Hardware typically limits to 32 blocks per operation
- **Validation**: Must be > 0 for data operations

### RPMB Partition Size Limits

**EXT_CSD Configuration**:
- **RPMB_SIZE_MULT** (EXT_CSD[168]): Multiplier for RPMB size
- **Size Formula**: RPMB Size = RPMB_SIZE_MULT × 128 KB
- **RPMB_SIZE_MULT Range**: 0 to 128 (8-bit field)
- **Maximum Capacity**: 128 × 128 KB = 16 MB

**Addressing Calculations**:
- **Maximum Addressable Blocks**: RPMB_SIZE / 256 bytes
- **16MB Example**: 16,777,216 ÷ 256 = 65,536 blocks
- **Address Range**: 0 to (total_blocks - 1)

### Validation Rules

1. **Partition Size Check**: 
   ```
   rpmb_size = EXT_CSD[RPMB_SIZE_MULT] × 128KB
   max_address = (rpmb_size / 256) - 1
   ```

2. **Address Bounds Validation**:
   ```
   if (address > max_address) return ADDRESS_ERROR;
   ```

3. **Block Count Validation**:
   ```
   if (block_count == 0) return INVALID_PARAM;
   if (block_count > IMPLEMENTATION_MAX) return INVALID_PARAM;
   ```

4. **Range Overflow Check**:
   ```
   if ((address + block_count) > (rpmb_size / 256)) return ADDRESS_ERROR;
   ```

5. **Address Alignment**: 
   - No sub-block addressing allowed
   - All operations must be on 256-byte boundaries

## Alignment Requirements

As specified in JESD84-B51, RPMB has specific alignment requirements for proper operation.

### Frame Alignment
- **Frame Size**: Each RPMB frame is exactly 512 bytes (one half-sector)
- **Physical Alignment**: RPMB frames align with eMMC 512-byte physical sectors
- **CMD Block Size**: CMD18/CMD25 block size must be 512 bytes for RPMB
- **Memory Alignment**: Host buffers should be aligned to cache line boundaries (32/64 bytes) for DMA efficiency

### Address Alignment  
- **Block Boundary**: All operations must be aligned to 256-byte half-sector boundaries
- **No Partial Blocks**: Cannot read/write partial 256-byte data blocks
- **Sequential Access**: Multi-block operations access consecutive half-sectors only
- **Address Units**: Address field represents half-sector index, not byte offset

### Data Structure Alignment
- **User Data**: 256 bytes per half-sector, no internal alignment restrictions
- **Multi-byte Fields**: All frame metadata fields use big-endian byte order
- **Structure Packing**: Frame structure must be packed without compiler padding
- **Field Boundaries**: All fields must maintain exact byte offsets as specified

### Implementation Alignment Considerations

1. **DMA Buffer Alignment**:
   ```c
   // Example: Align RPMB frame buffer to cache line
   __attribute__((aligned(64))) emmc_rpmb_frame_t rpmb_buffer[MAX_FRAMES];
   ```

2. **Endianness Handling**:
   ```c
   // All multi-byte fields must be converted to/from big-endian
   frame.address = htobe16(block_address);
   frame.block_count = htobe16(num_blocks);  
   frame.write_counter = htobe32(counter_value);
   ```

3. **Address Calculation**:
   ```c
   // Convert byte offset to half-sector address
   half_sector_address = byte_offset / 256;
   
   // Verify alignment
   if (byte_offset % 256 != 0) return ALIGNMENT_ERROR;
   ```

## Multi-Block Operations

Multi-block RPMB operations follow strict rules as defined in JESD84-B51 Section 6.6.22.

### Multi-Block Write Operations

**Frame Consistency Rules**:
1. **Write Counter**: ALL frames must use the SAME write counter value
2. **Address Sequence**: Frames address consecutive half-sectors (address, address+1, address+2, ...)
3. **Block Count**: ALL frames must have the same block_count value (total number of frames)
4. **Request Type**: ALL frames must have req_resp = 0x0003

**MAC Placement Rules**:
1. **Last Frame Only**: MAC field is set ONLY in the LAST frame
2. **Other Frames**: MAC field in frames 1 to N-1 must be filled with 0x00
3. **MAC Coverage**: Single MAC covers all frames' data and metadata

**Atomicity Guarantees**:
1. **All-or-Nothing**: Multi-block write is atomic (either all frames succeed or all fail)
2. **Counter Increment**: Write counter increments by 1 only after ALL frames are successfully written
3. **Reliable Write**: Must use CMD23 with reliable write flag set

### Multi-Block Read Operations

**Response Frame Rules**:
1. **Nonce Echo**: Device echoes request nonce in ALL response frames
2. **Address Sequence**: Response frames return consecutive half-sectors
3. **Block Count**: ALL frames contain the same block_count value
4. **Request Type**: ALL frames have req_resp = 0x0400

**MAC Verification Rules**:
1. **Last Frame Only**: MAC field is present ONLY in the LAST frame
2. **Other Frames**: MAC field in frames 1 to N-1 contains 0x00
3. **MAC Verification**: Single MAC verification covers all frames

### HMAC Calculation for Multi-Block

The HMAC calculation for multi-block operations covers ALL frames' data and metadata:

```
HMAC_SHA256_Input = 
    Frame₁_Data[256] || Frame₁_Metadata[30] ||
    Frame₂_Data[256] || Frame₂_Metadata[30] ||
    ...
    FrameN_Data[256] || FrameN_Metadata[30]

Where Frame_Metadata is exactly 30 bytes in this order:
1. nonce          (16 bytes, big-endian)
2. write_counter  (4 bytes, big-endian) 
3. address        (2 bytes, big-endian)
4. block_count    (2 bytes, big-endian)
5. result         (2 bytes, big-endian)
6. req_resp       (2 bytes, big-endian)

Total per frame: 256 + 30 = 286 bytes
Total for N frames: N × 286 bytes
```

### Implementation Requirements

**Write Operation Sequence**:
1. Get current write counter value
2. Prepare N frames with same counter, consecutive addresses
3. Calculate HMAC over all frames' data and metadata
4. Place MAC only in last frame
5. Send all frames via CMD25 with reliable write
6. Read result via result read sequence

**Read Operation Sequence**:  
1. Generate cryptographically random nonce
2. Send read request with starting address and count
3. Receive N response frames
4. Verify nonce echo in all frames
5. Verify MAC in last frame covers all frames
6. Extract data from all frames

**Error Handling**:
- Any frame error invalidates the entire multi-block operation
- Partial completion is not allowed
- MAC verification failure requires complete retry

## Security Requirements

RPMB security is based on cryptographic authentication and replay protection as specified in JESD84-B51.

### Authentication Key Management

**Key Properties**:
- **Size**: 256 bits (32 bytes) symmetric key
- **Programming**: One-time programmable during manufacturing or first use
- **Storage**: Stored in device OTP (One-Time Programmable) area
- **Access**: Key never exposed outside the device
- **Scope**: Single key used for all RPMB operations on the device

**Key Programming Requirements**:
1. Key can only be programmed once per device
2. Programming requires authentication key programming request (0x0001)
3. Key programming uses reliable write for atomicity
4. Failed key programming may render RPMB unusable

### HMAC-SHA256 Authentication

**Algorithm Specification**:
- **Standard**: HMAC-SHA256 as per RFC 2104 and FIPS 198-1
- **Key**: 256-bit authentication key stored in device
- **Hash Function**: SHA-256 (256-bit output)
- **MAC Size**: 256 bits (32 bytes)

**MAC Calculation Process**:
1. **Input Preparation**: Concatenate frame data and metadata in specified order
2. **HMAC Computation**: HMAC-SHA256(key, input_data)
3. **MAC Placement**: Place 32-byte MAC in key_mac field
4. **Verification**: Device/host independently calculates and compares MAC

### Replay Protection Mechanisms

**Write Counter Protection**:
- **Monotonic Property**: Counter only increases, never decreases or resets
- **Increment Rule**: Counter increments by exactly 1 after successful write
- **Bit Width**: 32-bit counter (4,294,967,296 maximum writes)
- **Overflow Handling**: Implementation-defined behavior at maximum value
- **Attack Detection**: Any counter rollback indicates replay attack

**Nonce-Based Protection**:
- **Generation**: Host generates cryptographically random 128-bit nonce
- **Usage**: Required for read operations, optional for counter reads
- **Echo Requirement**: Device must echo nonce exactly in responses
- **Uniqueness**: Each operation should use a unique nonce
- **Validation**: Host verifies echoed nonce matches sent nonce

### Cryptographic Requirements

**Random Number Generation**:
- **Quality**: Cryptographically secure pseudo-random number generator (CSPRNG)
- **Entropy**: Sufficient entropy for nonce generation
- **Uniqueness**: Low probability of nonce collision
- **Standards**: Follow NIST SP 800-90A or equivalent

**MAC Verification**:
- **Constant-Time Comparison**: Prevent timing attack vulnerabilities
- **Complete Verification**: Verify entire 32-byte MAC, not truncated
- **Error Handling**: Treat any MAC mismatch as authentication failure
- **No Partial Match**: Do not proceed with partial MAC matches

### Security Implementation Guidelines

**Timing Attack Prevention**:
```c
// Example: Constant-time MAC comparison
int secure_mac_compare(const uint8_t *mac1, const uint8_t *mac2) {
    volatile uint8_t result = 0;
    for (int i = 0; i < 32; i++) {
        result |= mac1[i] ^ mac2[i];
    }
    return result == 0;
}
```

**Key Protection**:
1. **Never Log Keys**: Authentication key must never appear in logs
2. **Memory Clearing**: Clear key material from temporary buffers
3. **Debug Protection**: Disable key exposure in debug builds
4. **Access Control**: Limit key access to authorized code only

**Counter Management**:
1. **Verification**: Always verify counter increment after writes
2. **Rollback Detection**: Detect and reject counter rollback attempts
3. **Monitoring**: Log suspicious counter behavior
4. **Recovery**: Define behavior for counter-related errors

**Error Handling Security**:
1. **Fail Securely**: Default to secure state on errors
2. **No Information Leakage**: Error messages must not leak sensitive data
3. **Attack Resilience**: Continue operation despite attack attempts
4. **Audit Trail**: Log security-relevant events for analysis

### Threat Model

**Protected Against**:
- Replay attacks (via write counter and nonce)
- Data tampering (via HMAC authentication)
- Unauthorized access (via authentication key)
- Eavesdropping (data confidentiality via external encryption if needed)

**Not Protected Against**:
- Physical device compromise (key extraction from hardware)
- Side-channel attacks (unless additional countermeasures implemented)  
- Brute force attacks on weak keys (use strong key generation)
- Social engineering attacks on key management

---

**Document Information**:
- **Source**: JEDEC Standard JESD84-B51 (February 2015)
- **Section**: 6.6.22 RPMB Partition  
- **Pages**: 166-176
- **Revision**: Based on official specification document

*For complete and authoritative information, refer to the official JEDEC Standard JESD84-B51 document available from JEDEC.*