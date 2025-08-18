#ifndef EMMC_REGS_H
#define EMMC_REGS_H

#include "emmc_types.h"

/* Generic eMMC controller register offsets */
/* These offsets are generic and may need adjustment for specific controllers */

/* System Address Register */
#define EMMC_SYSADDR 0x00

/* Block Size and Block Count Register */
#define EMMC_BLKSIZE 0x04
#define EMMC_BLKCNT 0x06

/* Argument Register */
#define EMMC_ARG 0x08

/* Transfer Mode and Command Register */
#define EMMC_XFER_MODE 0x0C
#define EMMC_COMMAND 0x0E

/* Response Registers */
#define EMMC_RESP0 0x10
#define EMMC_RESP1 0x14
#define EMMC_RESP2 0x18
#define EMMC_RESP3 0x1C

/* Buffer Data Port Register */
#define EMMC_DATA 0x20

/* Present State Register */
#define EMMC_PRNTS_STATE 0x24

/* Host Control and Power Control Register */
#define EMMC_HOST_CTRL 0x28
#define EMMC_PWR_CTRL 0x29

/* Block Gap Control and Wakeup Control Register */
#define EMMC_BLKGAP_CTRL 0x2A
#define EMMC_WAKEUP_CTRL 0x2B

/* Clock Control Register */
#define EMMC_CLK_CTRL 0x2C

/* Timeout Control and Software Reset Register */
#define EMMC_TIMEOUT_CTRL 0x2E
#define EMMC_SW_RESET 0x2F

/* Interrupt Status Register */
#define EMMC_INT_STATUS 0x30

/* Interrupt Status Enable Register */
#define EMMC_INT_STATUS_EN 0x34

/* Interrupt Signal Enable Register */
#define EMMC_INT_SIGNAL_EN 0x38

/* Auto CMD Error Status Register */
#define EMMC_ACMD_ERR_STATUS 0x3C

/* Host Control 2 Register */
#define EMMC_HOST_CTRL2 0x3E

/* Capabilities Register */
#define EMMC_CAPABILITIES 0x40
#define EMMC_CAPABILITIES_1 0x44

/* Maximum Current Capabilities Register */
#define EMMC_MAX_CURRENT 0x48

/* Force Event Register */
#define EMMC_FORCE_EVENT 0x50

/* ADMA Error Status Register */
#define EMMC_ADMA_ERR_STATUS 0x54

/* ADMA System Address Register */
#define EMMC_ADMA_ADDR 0x58

/* Preset Value Registers */
#define EMMC_PRESET_VAL_INIT 0x60
#define EMMC_PRESET_VAL_DS 0x62
#define EMMC_PRESET_VAL_HS 0x64
#define EMMC_PRESET_VAL_SDR12 0x66
#define EMMC_PRESET_VAL_SDR25 0x68
#define EMMC_PRESET_VAL_SDR50 0x6A
#define EMMC_PRESET_VAL_SDR104 0x6C
#define EMMC_PRESET_VAL_DDR50 0x6E

/* Host Controller Version Register */
#define EMMC_HOST_VERSION 0xFE

/* Register bit definitions */

/* EMMC_COMMAND register bits */
#define EMMC_CMD_RESP_TYPE_MASK 0x03
#define EMMC_CMD_RESP_TYPE_SHIFT 0
#define EMMC_CMD_CRC_CHECK_EN (1 << 3)
#define EMMC_CMD_IDX_CHECK_EN (1 << 4)
#define EMMC_CMD_DATA_PRESENT (1 << 5)
#define EMMC_CMD_TYPE_MASK (3 << 6)
#define EMMC_CMD_TYPE_NORMAL (0 << 6)
#define EMMC_CMD_TYPE_SUSPEND (1 << 6)
#define EMMC_CMD_TYPE_RESUME (2 << 6)
#define EMMC_CMD_TYPE_ABORT (3 << 6)
#define EMMC_CMD_IDX_MASK (0x3F << 8)
#define EMMC_CMD_IDX_SHIFT 8

/* EMMC_XFER_MODE register bits */
#define EMMC_XFER_DMA_EN (1 << 0)
#define EMMC_XFER_BLOCK_COUNT_EN (1 << 1)
#define EMMC_XFER_ACMD12_EN (1 << 2)
#define EMMC_XFER_ACMD23_EN (1 << 3)
#define EMMC_XFER_DATA_TO_HOST (1 << 4)
#define EMMC_XFER_MULTI_BLOCK (1 << 5)

/* EMMC_PRNTS_STATE register bits */
#define EMMC_STATE_CMD_INHIBIT (1 << 0)
#define EMMC_STATE_DATA_INHIBIT (1 << 1)
#define EMMC_STATE_DAT_ACTIVE (1 << 2)
#define EMMC_STATE_RETUNING_REQUEST (1 << 3)
#define EMMC_STATE_DAT0_SIGNAL_LEVEL (1 << 20)
#define EMMC_STATE_DAT1_SIGNAL_LEVEL (1 << 21)
#define EMMC_STATE_DAT2_SIGNAL_LEVEL (1 << 22)
#define EMMC_STATE_DAT3_SIGNAL_LEVEL (1 << 23)
#define EMMC_STATE_CMD_SIGNAL_LEVEL (1 << 24)
#define EMMC_STATE_CARD_INSERTED (1 << 16)
#define EMMC_STATE_CARD_STABLE (1 << 17)
#define EMMC_STATE_CARD_DETECT_LEVEL (1 << 18)
#define EMMC_STATE_WP_SWITCH_LEVEL (1 << 19)

/* EMMC_HOST_CTRL register bits */
#define EMMC_CTRL_LED_ON (1 << 0)
#define EMMC_CTRL_DATA_WIDTH_4BIT (1 << 1)
#define EMMC_CTRL_HIGH_SPEED (1 << 2)
#define EMMC_CTRL_DMA_MASK (3 << 3)
#define EMMC_CTRL_SDMA (0 << 3)
#define EMMC_CTRL_ADMA1 (1 << 3)
#define EMMC_CTRL_ADMA2 (2 << 3)
#define EMMC_CTRL_CARD_DETECT_TEST (1 << 6)
#define EMMC_CTRL_CARD_DETECT_SIGNAL (1 << 7)

/* EMMC_PWR_CTRL register bits */
#define EMMC_PWR_CTRL_POWER_ON (1 << 0)
#define EMMC_PWR_CTRL_VOLT_MASK (7 << 1)
#define EMMC_PWR_CTRL_VOLT_18V (5 << 1)
#define EMMC_PWR_CTRL_VOLT_30V (6 << 1)
#define EMMC_PWR_CTRL_VOLT_33V (7 << 1)

/* EMMC_CLK_CTRL register bits */
#define EMMC_CLK_CTRL_INTERNAL_EN (1 << 0)
#define EMMC_CLK_CTRL_STABLE (1 << 1)
#define EMMC_CLK_CTRL_CARD_EN (1 << 2)
#define EMMC_CLK_CTRL_PLL_EN (1 << 3)
#define EMMC_CLK_CTRL_DIV_MASK (0xFF << 8)
#define EMMC_CLK_CTRL_DIV_SHIFT 8

/* EMMC_SW_RESET register bits */
#define EMMC_RESET_ALL (1 << 0)
#define EMMC_RESET_CMD (1 << 1)
#define EMMC_RESET_DATA (1 << 2)

/* EMMC_INT_STATUS register bits */
#define EMMC_INT_CMD_COMPLETE (1 << 0)
#define EMMC_INT_DATA_COMPLETE (1 << 1)
#define EMMC_INT_BLOCK_GAP (1 << 2)
#define EMMC_INT_DMA_COMPLETE (1 << 3)
#define EMMC_INT_SPACE_AVAILABLE (1 << 4)
#define EMMC_INT_DATA_AVAILABLE (1 << 5)
#define EMMC_INT_CARD_INSERT (1 << 6)
#define EMMC_INT_CARD_REMOVE (1 << 7)
#define EMMC_INT_CARD_INTERRUPT (1 << 8)
#define EMMC_INT_INT_A (1 << 9)
#define EMMC_INT_INT_B (1 << 10)
#define EMMC_INT_INT_C (1 << 11)
#define EMMC_INT_RETUNE (1 << 12)
#define EMMC_INT_BOOT_ACK_RECV (1 << 13)
#define EMMC_INT_BOOT_COMPLETE (1 << 14)
#define EMMC_INT_ERROR (1 << 15)
#define EMMC_INT_TIMEOUT (1 << 16)
#define EMMC_INT_CRC_ERROR (1 << 17)
#define EMMC_INT_END_BIT_ERROR (1 << 18)
#define EMMC_INT_INDEX_ERROR (1 << 19)
#define EMMC_INT_DATA_TIMEOUT (1 << 20)
#define EMMC_INT_DATA_CRC_ERROR (1 << 21)
#define EMMC_INT_DATA_END_BIT_ERROR (1 << 22)
#define EMMC_INT_CURRENT_LIMIT_ERROR (1 << 23)
#define EMMC_INT_ACMD_ERROR (1 << 24)
#define EMMC_INT_ADMA_ERROR (1 << 25)
#define EMMC_INT_TUNING_ERROR (1 << 26)
#define EMMC_INT_RESP_ERROR (1 << 27)
#define EMMC_INT_BOOT_ACK_ERROR (1 << 28)
#define EMMC_INT_VENDOR_ERROR_MASK (0xF << 28)

/* EMMC_HOST_CTRL2 register bits */
#define EMMC_CTRL2_UHS_MODE_MASK (7 << 0)
#define EMMC_CTRL2_UHS_SDR12 (0 << 0)
#define EMMC_CTRL2_UHS_SDR25 (1 << 0)
#define EMMC_CTRL2_UHS_SDR50 (2 << 0)
#define EMMC_CTRL2_UHS_SDR104 (3 << 0)
#define EMMC_CTRL2_UHS_DDR50 (4 << 0)
#define EMMC_CTRL2_HS400_MODE (5 << 0)
#define EMMC_CTRL2_HS400_ES_MODE (7 << 0) /* HS400 Enhanced Strobe */
#define EMMC_CTRL2_18V_ENABLE (1 << 3)
#define EMMC_CTRL2_DRV_STRENGTH_MASK (3 << 4)
#define EMMC_CTRL2_DRV_STRENGTH_B (0 << 4)
#define EMMC_CTRL2_DRV_STRENGTH_A (1 << 4)
#define EMMC_CTRL2_DRV_STRENGTH_C (2 << 4)
#define EMMC_CTRL2_DRV_STRENGTH_D (3 << 4)
#define EMMC_CTRL2_EXECUTE_TUNING (1 << 6)
#define EMMC_CTRL2_SAMPLE_CLK_SEL (1 << 7)
#define EMMC_CTRL2_UHS2_IF_EN (1 << 8)
#define EMMC_CTRL2_ADMA2_LEN_MODE (1 << 10)
#define EMMC_CTRL2_CMD23_EN (1 << 11)
#define EMMC_CTRL2_HOST_VER4_EN (1 << 12)
#define EMMC_CTRL2_ADDR64_EN (1 << 13)
#define EMMC_CTRL2_ASYNC_INT_EN (1 << 14)
#define EMMC_CTRL2_PRESET_VAL_EN (1 << 15)

/* Enhanced Strobe Control */
#define EMMC_CTRL2_ENHANCED_STROBE_EN (1 << 16) /* Enhanced strobe enable */

/* Common register masks */
#define EMMC_ERROR_INT_MASK (EMMC_INT_TIMEOUT | EMMC_INT_CRC_ERROR |             \
							 EMMC_INT_END_BIT_ERROR | EMMC_INT_INDEX_ERROR |     \
							 EMMC_INT_DATA_TIMEOUT | EMMC_INT_DATA_CRC_ERROR |   \
							 EMMC_INT_DATA_END_BIT_ERROR | EMMC_INT_ACMD_ERROR | \
							 EMMC_INT_ADMA_ERROR)

#define EMMC_NORMAL_INT_MASK (EMMC_INT_CMD_COMPLETE | EMMC_INT_DATA_COMPLETE |   \
							  EMMC_INT_DMA_COMPLETE | EMMC_INT_SPACE_AVAILABLE | \
							  EMMC_INT_DATA_AVAILABLE | EMMC_INT_CARD_INSERT |   \
							  EMMC_INT_CARD_REMOVE)

#define EMMC_ALL_INT_MASK (EMMC_ERROR_INT_MASK | EMMC_NORMAL_INT_MASK)

#endif /* EMMC_REGS_H */
