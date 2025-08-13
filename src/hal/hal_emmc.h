#ifndef HAL_EMMC_H
#define HAL_EMMC_H

#include "../include/emmc_types.h"
#include "../include/emmc_cmd.h"
#include "../include/emmc_regs.h"

/* HAL initialization and configuration */
typedef struct {
    u32 base_address;       /* eMMC controller base address */
    u32 max_clock_freq;     /* Maximum supported clock frequency */
    u32 timeout_ms;         /* Command timeout in milliseconds */
    bool dma_enabled;       /* DMA support flag */
    u8  max_bus_width;      /* Maximum bus width (1, 4, or 8) */
} hal_emmc_config_t;

/* eMMC command structure */
typedef struct {
    u8  index;              /* Command index (0-63) */
    u32 argument;           /* Command argument */
    emmc_resp_type_t resp_type; /* Expected response type */
    bool data_present;      /* Data transfer expected */
    bool crc_check;         /* Enable CRC checking */
    bool index_check;       /* Enable index checking */
} hal_emmc_cmd_t;

/* Data transfer structure */
typedef struct {
    u8   *buffer;           /* Data buffer pointer */
    u32  block_size;        /* Block size in bytes */
    u32  block_count;       /* Number of blocks */
    bool read_operation;    /* true for read, false for write */
    bool use_dma;          /* Use DMA for transfer */
} hal_emmc_data_t;

/* HAL function prototypes */

/**
 * @brief Initialize the eMMC controller hardware
 * @param config HAL configuration structure
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t hal_emmc_init(const hal_emmc_config_t *config);

/**
 * @brief Reset the eMMC controller
 * @param reset_type Type of reset (all, cmd, data)
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t hal_emmc_reset(u8 reset_type);

/**
 * @brief Set the eMMC clock frequency
 * @param frequency Desired clock frequency in Hz
 * @return Actual frequency set, or 0 on error
 */
u32 hal_emmc_set_clock(u32 frequency);

/**
 * @brief Enable or disable the eMMC clock
 * @param enable true to enable, false to disable
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t hal_emmc_clock_enable(bool enable);

/**
 * @brief Set the bus width
 * @param width Bus width (1, 4, or 8 bits)
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t hal_emmc_set_bus_width(emmc_bus_width_t width);

/**
 * @brief Set the bus timing mode
 * @param mode Timing mode (legacy, HS, HS200, HS400)
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t hal_emmc_set_timing(emmc_bus_mode_t mode);

/**
 * @brief Send a command to the eMMC card
 * @param cmd Command structure
 * @param data Data structure (NULL if no data transfer)
 * @return EMMC_OK on success, error code otherwise
 * 
 * HAL Implementation Guide:
 * When data is provided, this function should handle all data transfer
 * internally, choosing between DMA and PIO based on data->use_dma flag:
 * 
 * Example implementation:
 *   if (data) {
 *     if (data->use_dma) {
 *       setup_dma_transfer(data->buffer, data->block_size * data->block_count);
 *       start_dma_and_wait_complete();
 *     } else {
 *       if (data->read_operation) {
 *         pio_read_blocks(data->buffer, data->block_size, data->block_count);
 *       } else {
 *         pio_write_blocks(data->buffer, data->block_size, data->block_count);
 *       }
 *     }
 *   }
 */
emmc_result_t hal_emmc_send_command(const hal_emmc_cmd_t *cmd, 
                                   const hal_emmc_data_t *data);

/**
 * @brief Get the response from the last command
 * @param response Buffer to store response (16 bytes for R2, 4 bytes for others)
 * @param resp_type Expected response type
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t hal_emmc_get_response(u32 *response, emmc_resp_type_t resp_type);

/**
 * @brief Wait for data transfer completion
 * @param timeout_ms Timeout in milliseconds
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t hal_emmc_wait_data_complete(u32 timeout_ms);

/*
 * Note: Data transfer is handled internally by hal_emmc_send_command()
 * when data parameter is provided. The HAL implementation should handle
 * both DMA and PIO modes based on the use_dma flag in hal_emmc_data_t.
 * No separate read_data/write_data functions are needed.
 */

/**
 * @brief Check if the controller is ready for a new command
 * @return true if ready, false if busy
 */
bool hal_emmc_is_command_ready(void);

/**
 * @brief Check if data transfer is in progress
 * @return true if transfer active, false otherwise
 */
bool hal_emmc_is_data_active(void);

/**
 * @brief Get the current interrupt status
 * @return Interrupt status register value
 */
u32 hal_emmc_get_interrupt_status(void);

/**
 * @brief Clear interrupt status bits
 * @param status Interrupt status bits to clear
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t hal_emmc_clear_interrupt_status(u32 status);

/**
 * @brief Enable specific interrupts
 * @param mask Interrupt mask to enable
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t hal_emmc_enable_interrupts(u32 mask);

/**
 * @brief Disable specific interrupts
 * @param mask Interrupt mask to disable
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t hal_emmc_disable_interrupts(u32 mask);

/**
 * @brief Get the present state register
 * @return Present state register value
 */
u32 hal_emmc_get_present_state(void);

/**
 * @brief Set power control
 * @param power_on true to power on, false to power off
 * @param voltage Voltage level (1.8V, 3.0V, 3.3V)
 * @return EMMC_OK on success, error code otherwise
 */
emmc_result_t hal_emmc_set_power(bool power_on, u8 voltage);

/**
 * @brief Perform platform-specific delay
 * @param microseconds Delay time in microseconds
 */
void hal_emmc_delay_us(u32 microseconds);

/**
 * @brief Perform platform-specific delay
 * @param milliseconds Delay time in milliseconds
 */
void hal_emmc_delay_ms(u32 milliseconds);

/**
 * @brief Read a 32-bit register
 * @param offset Register offset from base address
 * @return Register value
 */
u32 hal_emmc_read_reg(u32 offset);

/**
 * @brief Write a 32-bit register
 * @param offset Register offset from base address
 * @param value Value to write
 */
void hal_emmc_write_reg(u32 offset, u32 value);

/**
 * @brief Read a 16-bit register
 * @param offset Register offset from base address
 * @return Register value
 */
u16 hal_emmc_read_reg16(u32 offset);

/**
 * @brief Write a 16-bit register
 * @param offset Register offset from base address
 * @param value Value to write
 */
void hal_emmc_write_reg16(u32 offset, u16 value);

/**
 * @brief Read an 8-bit register
 * @param offset Register offset from base address
 * @return Register value
 */
u8 hal_emmc_read_reg8(u32 offset);

/**
 * @brief Write an 8-bit register
 * @param offset Register offset from base address
 * @param value Value to write
 */
void hal_emmc_write_reg8(u32 offset, u8 value);

/* Inline helper functions for common register access patterns */

/**
 * @brief Wait for a register bit to become set
 * @param offset Register offset
 * @param mask Bit mask to check
 * @param timeout_ms Timeout in milliseconds
 * @return EMMC_OK if bit set within timeout, EMMC_TIMEOUT otherwise
 */
static inline emmc_result_t hal_emmc_wait_for_bit_set(u32 offset, u32 mask, u32 timeout_ms)
{
    u32 timeout = timeout_ms * 1000;  /* Convert to microseconds */
    
    while (timeout > 0) {
        if (hal_emmc_read_reg(offset) & mask) {
            return EMMC_OK;
        }
        hal_emmc_delay_us(10);
        timeout -= 10;
    }
    
    return EMMC_TIMEOUT;
}

/**
 * @brief Wait for a register bit to become clear
 * @param offset Register offset
 * @param mask Bit mask to check
 * @param timeout_ms Timeout in milliseconds
 * @return EMMC_OK if bit cleared within timeout, EMMC_TIMEOUT otherwise
 */
static inline emmc_result_t hal_emmc_wait_for_bit_clear(u32 offset, u32 mask, u32 timeout_ms)
{
    u32 timeout = timeout_ms * 1000;  /* Convert to microseconds */
    
    while (timeout > 0) {
        if (!(hal_emmc_read_reg(offset) & mask)) {
            return EMMC_OK;
        }
        hal_emmc_delay_us(10);
        timeout -= 10;
    }
    
    return EMMC_TIMEOUT;
}

/* Hardware-specific constants that may need adjustment per platform */
#define HAL_EMMC_DEFAULT_TIMEOUT_MS     5000
#define HAL_EMMC_INIT_CLOCK_FREQ        400000      /* 400 kHz */
#define HAL_EMMC_HS_CLOCK_FREQ          26000000    /* 26 MHz */
#define HAL_EMMC_HS200_CLOCK_FREQ       200000000   /* 200 MHz */
#define HAL_EMMC_HS400_CLOCK_FREQ       200000000   /* 200 MHz (DDR) */

/* Voltage definitions */
#define HAL_EMMC_VOLTAGE_18V            5
#define HAL_EMMC_VOLTAGE_30V            6
#define HAL_EMMC_VOLTAGE_33V            7

#endif /* HAL_EMMC_H */