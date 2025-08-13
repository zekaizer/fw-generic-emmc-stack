/*
 * HAL Stub Implementation for Testing
 * 
 * This file provides a basic stub implementation of the HAL interface
 * for testing purposes. In a real implementation, these functions would
 * access actual hardware registers.
 */

#include "../src/hal/hal_emmc.h"

/* Simulated register state */
static struct {
    u32 base_addr;
    u32 timeout_ms;
    u32 clock_freq;
    bool clock_enabled;
    bool power_on;
    emmc_bus_width_t bus_width;
    emmc_bus_mode_t timing_mode;
    bool initialized;
    
    /* Simulated registers */
    u32 present_state;
    u32 interrupt_status;
    u32 response[4];
    u8  data_buffer[4096];
    u32 data_index;
} g_hal_state = {0};

emmc_result_t hal_emmc_init(const hal_emmc_config_t *config)
{
    if (!config) {
        return EMMC_INVALID_PARAM;
    }
    
    g_hal_state.base_addr = config->base_address;
    g_hal_state.timeout_ms = config->timeout_ms;
    g_hal_state.clock_freq = 0;
    g_hal_state.clock_enabled = false;
    g_hal_state.power_on = false;
    g_hal_state.bus_width = EMMC_BUS_WIDTH_1;
    g_hal_state.timing_mode = EMMC_MODE_SDR;
    g_hal_state.initialized = true;
    
    /* Initialize simulated registers */
    g_hal_state.present_state = 0;
    g_hal_state.interrupt_status = 0;
    g_hal_state.data_index = 0;
    
    return EMMC_OK;
}

emmc_result_t hal_emmc_reset(u8 reset_type)
{
    if (!g_hal_state.initialized) {
        return EMMC_NOT_READY;
    }
    
    /* Simulate reset by clearing appropriate state */
    if (reset_type & EMMC_RESET_ALL) {
        g_hal_state.present_state = 0;
        g_hal_state.interrupt_status = 0;
        g_hal_state.data_index = 0;
    }
    
    /* Simulate reset completion delay */
    hal_emmc_delay_ms(1);
    
    return EMMC_OK;
}

u32 hal_emmc_set_clock(u32 frequency)
{
    if (!g_hal_state.initialized) {
        return 0;
    }
    
    /* Simulate clock setting with some limitations */
    if (frequency > 200000000) {
        frequency = 200000000; /* Cap at 200MHz */
    }
    if (frequency < 400000) {
        frequency = 400000; /* Minimum 400kHz */
    }
    
    g_hal_state.clock_freq = frequency;
    return frequency;
}

emmc_result_t hal_emmc_clock_enable(bool enable)
{
    if (!g_hal_state.initialized) {
        return EMMC_NOT_READY;
    }
    
    g_hal_state.clock_enabled = enable;
    
    /* Simulate clock stabilization delay */
    if (enable) {
        hal_emmc_delay_ms(2);
    }
    
    return EMMC_OK;
}

emmc_result_t hal_emmc_set_bus_width(emmc_bus_width_t width)
{
    if (!g_hal_state.initialized) {
        return EMMC_NOT_READY;
    }
    
    g_hal_state.bus_width = width;
    return EMMC_OK;
}

emmc_result_t hal_emmc_set_timing(emmc_bus_mode_t mode)
{
    if (!g_hal_state.initialized) {
        return EMMC_NOT_READY;
    }
    
    g_hal_state.timing_mode = mode;
    return EMMC_OK;
}

emmc_result_t hal_emmc_send_command(const hal_emmc_cmd_t *cmd, 
                                   const hal_emmc_data_t *data)
{
    if (!g_hal_state.initialized || !cmd) {
        return EMMC_INVALID_PARAM;
    }
    
    if (!g_hal_state.power_on || !g_hal_state.clock_enabled) {
        return EMMC_NOT_READY;
    }
    
    /* Simulate command processing */
    hal_emmc_delay_us(100);
    
    /* Generate simulated responses based on command */
    switch (cmd->index) {
        case EMMC_CMD0: /* GO_IDLE_STATE */
            /* No response */
            break;
            
        case EMMC_CMD1: /* SEND_OP_COND */
            g_hal_state.response[0] = EMMC_OCR_CARD_POWER_UP | EMMC_OCR_VDD_33_34 | EMMC_OCR_SECTOR_MODE;
            break;
            
        case EMMC_CMD2: /* ALL_SEND_CID */
            /* Simulate CID response */
            g_hal_state.response[0] = 0x12345678;
            g_hal_state.response[1] = 0x9ABCDEF0;
            g_hal_state.response[2] = 0x11223344;
            g_hal_state.response[3] = 0x55667788;
            break;
            
        case EMMC_CMD3: /* SET_RELATIVE_ADDR */
            g_hal_state.response[0] = (cmd->argument & 0xFFFF0000) | 0x0700; /* Ready state */
            break;
            
        case EMMC_CMD7: /* SELECT_CARD */
            g_hal_state.response[0] = 0x00000700; /* Transfer state */
            break;
            
        case EMMC_CMD8: /* SEND_EXT_CSD */
            if (data && data->read_operation) {
                /* Simulate EXT_CSD data - copy directly to user buffer */
                for (u32 i = 0; i < 512; i++) {
                    data->buffer[i] = (u8)(i & 0xFF);
                }
                /* Set some important EXT_CSD fields */
                data->buffer[192] = 8; /* EXT_CSD_REV */
                data->buffer[196] = 0x07; /* CARD_TYPE - supports HS200 */
                data->buffer[212] = 0x00; /* SEC_COUNT[0] */
                data->buffer[213] = 0x00; /* SEC_COUNT[1] */
                data->buffer[214] = 0x10; /* SEC_COUNT[2] */
                data->buffer[215] = 0x00; /* SEC_COUNT[3] = 1M sectors */
            }
            g_hal_state.response[0] = 0x00000900; /* Ready for data */
            break;
            
        case EMMC_CMD9: /* SEND_CSD */
            /* Simulate CSD response */
            g_hal_state.response[0] = 0x400E0032;
            g_hal_state.response[1] = 0x5B590000;
            g_hal_state.response[2] = 0x00007F80;
            g_hal_state.response[3] = 0x0A4040FF;
            break;
            
        case EMMC_CMD13: /* SEND_STATUS */
            g_hal_state.response[0] = 0x00000900; /* Ready for data, transfer state */
            break;
            
        case EMMC_CMD17: /* READ_SINGLE_BLOCK */
        case EMMC_CMD18: /* READ_MULTIPLE_BLOCK */
            if (data && data->read_operation) {
                /* Simulate reading data pattern - copy directly to user buffer */
                for (u32 i = 0; i < data->block_size * data->block_count; i++) {
                    data->buffer[i] = (u8)((cmd->argument + i) & 0xFF);
                }
            } else if (data && !data->read_operation) {
                /* Simulate writing data pattern - copy from user buffer */
                for (u32 i = 0; i < data->block_size * data->block_count; i++) {
                    g_hal_state.data_buffer[i % sizeof(g_hal_state.data_buffer)] = data->buffer[i];
                }
            }
            g_hal_state.response[0] = 0x00000900;
            break;
            
        case EMMC_CMD24: /* WRITE_BLOCK */
        case EMMC_CMD25: /* WRITE_MULTIPLE_BLOCK */
            if (data && !data->read_operation) {
                /* Simulate writing data pattern - copy from user buffer */
                for (u32 i = 0; i < data->block_size * data->block_count; i++) {
                    g_hal_state.data_buffer[i % sizeof(g_hal_state.data_buffer)] = data->buffer[i];
                }
            }
            g_hal_state.response[0] = 0x00000900;
            break;
            
        case EMMC_CMD23: /* SET_BLOCK_COUNT */
            g_hal_state.response[0] = 0x00000900;
            break;
            
        case EMMC_CMD6: /* SWITCH */
            /* Simulate switch command delay */
            hal_emmc_delay_ms(10);
            g_hal_state.response[0] = 0x00000900;
            break;
            
        default:
            g_hal_state.response[0] = 0x00000900;
            break;
    }
    
    /* Set interrupt status to indicate command complete */
    g_hal_state.interrupt_status |= EMMC_INT_CMD_COMPLETE;
    if (data) {
        g_hal_state.interrupt_status |= EMMC_INT_DATA_COMPLETE;
    }
    
    return EMMC_OK;
}

emmc_result_t hal_emmc_get_response(u32 *response, emmc_resp_type_t resp_type)
{
    if (!g_hal_state.initialized || !response) {
        return EMMC_INVALID_PARAM;
    }
    
    switch (resp_type) {
        case EMMC_RESP_R2:
            response[0] = g_hal_state.response[0];
            response[1] = g_hal_state.response[1];
            response[2] = g_hal_state.response[2];
            response[3] = g_hal_state.response[3];
            break;
            
        case EMMC_RESP_R1:
        case EMMC_RESP_R1B:
        case EMMC_RESP_R3:
        case EMMC_RESP_R6:
        case EMMC_RESP_R7:
            response[0] = g_hal_state.response[0];
            break;
            
        default:
            return EMMC_INVALID_PARAM;
    }
    
    return EMMC_OK;
}

emmc_result_t hal_emmc_wait_data_complete(u32 timeout_ms)
{
    if (!g_hal_state.initialized) {
        return EMMC_NOT_READY;
    }
    
    /* Simulate data transfer delay */
    hal_emmc_delay_ms(1);
    
    /* Always succeed in stub */
    return EMMC_OK;
}

/*
 * Note: hal_emmc_read_data() and hal_emmc_write_data() functions have been
 * removed as data transfer is now handled internally by hal_emmc_send_command()
 * when a data structure is provided. The HAL implementation handles both
 * DMA and PIO modes automatically based on the use_dma flag.
 */

bool hal_emmc_is_command_ready(void)
{
    return g_hal_state.initialized && g_hal_state.power_on && g_hal_state.clock_enabled;
}

bool hal_emmc_is_data_active(void)
{
    return false; /* Always ready in stub */
}

u32 hal_emmc_get_interrupt_status(void)
{
    return g_hal_state.interrupt_status;
}

emmc_result_t hal_emmc_clear_interrupt_status(u32 status)
{
    if (!g_hal_state.initialized) {
        return EMMC_NOT_READY;
    }
    
    g_hal_state.interrupt_status &= ~status;
    return EMMC_OK;
}

emmc_result_t hal_emmc_enable_interrupts(u32 mask)
{
    /* Stub - just return success */
    return EMMC_OK;
}

emmc_result_t hal_emmc_disable_interrupts(u32 mask)
{
    /* Stub - just return success */
    return EMMC_OK;
}

u32 hal_emmc_get_present_state(void)
{
    return g_hal_state.present_state;
}

emmc_result_t hal_emmc_set_power(bool power_on, u8 voltage)
{
    if (!g_hal_state.initialized) {
        return EMMC_NOT_READY;
    }
    
    g_hal_state.power_on = power_on;
    
    /* Simulate power-up delay */
    if (power_on) {
        hal_emmc_delay_ms(5);
    }
    
    return EMMC_OK;
}

void hal_emmc_delay_us(u32 microseconds)
{
    /* Stub - in real implementation, this would be platform-specific delay */
    /* For testing, we can use a simple loop or platform sleep function */
    volatile u32 count = microseconds * 10; /* Rough approximation */
    while (count--) {
        /* Busy wait */
    }
}

void hal_emmc_delay_ms(u32 milliseconds)
{
    hal_emmc_delay_us(milliseconds * 1000);
}

u32 hal_emmc_read_reg(u32 offset)
{
    /* Simulate register read from base address + offset */
    /* In real implementation, this would access hardware registers */
    return 0;
}

void hal_emmc_write_reg(u32 offset, u32 value)
{
    /* Simulate register write to base address + offset */
    /* In real implementation, this would access hardware registers */
}

u16 hal_emmc_read_reg16(u32 offset)
{
    return (u16)(hal_emmc_read_reg(offset) & 0xFFFF);
}

void hal_emmc_write_reg16(u32 offset, u16 value)
{
    hal_emmc_write_reg(offset, (u32)value);
}

u8 hal_emmc_read_reg8(u32 offset)
{
    return (u8)(hal_emmc_read_reg(offset) & 0xFF);
}

void hal_emmc_write_reg8(u32 offset, u8 value)
{
    hal_emmc_write_reg(offset, (u32)value);
}