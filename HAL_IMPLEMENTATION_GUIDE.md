# HAL Implementation Guide

This guide explains how to implement the eMMC HAL (Hardware Abstraction Layer) for your specific platform.

## Overview

The HAL provides a platform-independent interface between the eMMC driver and the actual hardware controller. You need to implement the functions declared in `src/hal/hal_emmc.h` for your specific hardware platform.

## Key Design Changes

### Simplified Data Transfer

**Important:** The `hal_emmc_read_data()` and `hal_emmc_write_data()` functions have been **removed** from the interface. All data transfers are now handled internally by `hal_emmc_send_command()` when a data structure is provided.

## Core Functions to Implement

### 1. Initialization and Configuration

```c
emmc_result_t hal_emmc_init(const hal_emmc_config_t *config);
emmc_result_t hal_emmc_reset(u8 reset_type);
u32 hal_emmc_set_clock(u32 frequency);
emmc_result_t hal_emmc_clock_enable(bool enable);
emmc_result_t hal_emmc_set_power(bool power_on, u8 voltage);
```

### 2. Bus Configuration

```c
emmc_result_t hal_emmc_set_bus_width(emmc_bus_width_t width);
emmc_result_t hal_emmc_set_timing(emmc_bus_mode_t mode);
```

### 3. Command and Data Transfer (Most Important)

```c
emmc_result_t hal_emmc_send_command(const hal_emmc_cmd_t *cmd, 
                                   const hal_emmc_data_t *data);
```

**This function must handle:**
- Command transmission to eMMC card
- **All data transfers** (when data parameter is provided)
- Both DMA and PIO modes based on `data->use_dma` flag

### Example Implementation Pattern

```c
emmc_result_t hal_emmc_send_command(const hal_emmc_cmd_t *cmd, 
                                   const hal_emmc_data_t *data)
{
    /* 1. Send command */
    write_command_to_controller(cmd);
    wait_for_command_complete();
    
    /* 2. Handle data transfer if needed */
    if (data) {
        if (data->use_dma) {
            /* DMA Mode */
            setup_dma_descriptor(data->buffer, 
                               data->block_size * data->block_count,
                               data->read_operation);
            start_dma_transfer();
            wait_for_dma_complete();
        } else {
            /* PIO Mode */
            if (data->read_operation) {
                pio_read_blocks(data->buffer, data->block_size, data->block_count);
            } else {
                pio_write_blocks(data->buffer, data->block_size, data->block_count);
            }
        }
    }
    
    return EMMC_OK;
}
```

### PIO Implementation Example

```c
static void pio_read_blocks(u8 *buffer, u32 block_size, u32 block_count)
{
    u32 total_bytes = block_size * block_count;
    u32 *buf32 = (u32*)buffer;
    
    for (u32 i = 0; i < total_bytes; i += 4) {
        /* Wait for buffer read enable */
        while (!(read_reg(EMMC_PRESENT_STATE) & EMMC_BUFFER_READ_ENABLE))
            ;
        
        /* Read 4 bytes from data port */
        *buf32++ = read_reg(EMMC_BUFFER_DATA_PORT);
    }
}

static void pio_write_blocks(const u8 *buffer, u32 block_size, u32 block_count)
{
    u32 total_bytes = block_size * block_count;
    const u32 *buf32 = (const u32*)buffer;
    
    for (u32 i = 0; i < total_bytes; i += 4) {
        /* Wait for buffer write enable */
        while (!(read_reg(EMMC_PRESENT_STATE) & EMMC_BUFFER_WRITE_ENABLE))
            ;
        
        /* Write 4 bytes to data port */
        write_reg(EMMC_BUFFER_DATA_PORT, *buf32++);
    }
}
```

### 4. Status and Control Functions

```c
emmc_result_t hal_emmc_get_response(u32 *response, emmc_resp_type_t resp_type);
emmc_result_t hal_emmc_wait_data_complete(u32 timeout_ms);
bool hal_emmc_is_command_ready(void);
bool hal_emmc_is_data_active(void);
u32 hal_emmc_get_interrupt_status(void);
emmc_result_t hal_emmc_clear_interrupt_status(u32 status);
```

### 5. Register Access Functions

```c
u32 hal_emmc_read_reg(u32 offset);
void hal_emmc_write_reg(u32 offset, u32 value);
u16 hal_emmc_read_reg16(u32 offset);
void hal_emmc_write_reg16(u32 offset, u16 value);
u8 hal_emmc_read_reg8(u32 offset);
void hal_emmc_write_reg8(u32 offset, u8 value);
```

### 6. Platform-Specific Utilities

```c
void hal_emmc_delay_us(u32 microseconds);
void hal_emmc_delay_ms(u32 milliseconds);
```

## Platform-Specific Considerations

### ARM Platforms
- Use proper memory barriers for register access
- Consider cache coherency for DMA buffers
- Implement proper interrupt handling if needed

### x86 Platforms  
- Use appropriate I/O port or memory-mapped access
- Handle endianness if necessary

### RISC-V Platforms
- Follow RISC-V memory ordering requirements
- Use appropriate fence instructions

## Testing Your Implementation

Use the provided HAL stub (`tests/hal_stub.c`) as a reference. The stub shows how data transfers should be handled internally within `hal_emmc_send_command()`.

## Migration from Previous Versions

If you previously implemented `hal_emmc_read_data()` and `hal_emmc_write_data()`, you need to:

1. Remove these function implementations
2. Move the data transfer logic into `hal_emmc_send_command()`
3. Handle both DMA and PIO modes within the single function

## Common Pitfalls

1. **Don't** implement separate read_data/write_data functions
2. **Do** handle all data transfers in hal_emmc_send_command()
3. **Do** respect the use_dma flag for mode selection
4. **Do** implement proper timeout handling
5. **Do** use proper memory barriers for register access

## Platform Examples

Refer to the community wiki for platform-specific implementation examples:
- ARM Cortex-A series
- ARM Cortex-M series
- x86/x64 platforms
- RISC-V implementations