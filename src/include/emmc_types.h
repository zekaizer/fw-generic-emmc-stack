#ifndef EMMC_TYPES_H
#define EMMC_TYPES_H

#include <stdint.h>
#include <stdbool.h>

/* Basic type definitions */
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t s8;

/* eMMC response types */
typedef enum {
	EMMC_RESP_NONE = 0,
	EMMC_RESP_R1 = 1,
	EMMC_RESP_R1B = 2,
	EMMC_RESP_R2 = 3,
	EMMC_RESP_R3 = 4,
	EMMC_RESP_R4 = 5,
	EMMC_RESP_R5 = 6,
	EMMC_RESP_R6 = 7,
	EMMC_RESP_R7 = 8
} emmc_resp_type_t;

/* eMMC card states */
typedef enum {
	EMMC_STATE_IDLE = 0,
	EMMC_STATE_READY = 1,
	EMMC_STATE_IDENT = 2,
	EMMC_STATE_STBY = 3,
	EMMC_STATE_TRAN = 4,
	EMMC_STATE_DATA = 5,
	EMMC_STATE_RCV = 6,
	EMMC_STATE_PRG = 7,
	EMMC_STATE_DIS = 8,
	EMMC_STATE_BTST = 9,
	EMMC_STATE_SLP = 10
} emmc_state_t;

/* eMMC partitions */
typedef enum {
	EMMC_PART_USER = 0,
	EMMC_PART_BOOT1 = 1,
	EMMC_PART_BOOT2 = 2,
	EMMC_PART_RPMB = 3,
	EMMC_PART_GP1 = 4,
	EMMC_PART_GP2 = 5,
	EMMC_PART_GP3 = 6,
	EMMC_PART_GP4 = 7
} emmc_partition_t;

/* eMMC bus modes */
typedef enum {
	EMMC_MODE_SDR = 0,
	EMMC_MODE_DDR = 1,
	EMMC_MODE_HS200 = 2,
	EMMC_MODE_HS400 = 3,
	EMMC_MODE_HS400_ES = 4 /* Enhanced Strobe - highest performance mode */
} emmc_bus_mode_t;

/* eMMC bus widths */
typedef enum {
	EMMC_BUS_WIDTH_1 = 0,
	EMMC_BUS_WIDTH_4 = 1,
	EMMC_BUS_WIDTH_8 = 2
} emmc_bus_width_t;

/* Return codes */
typedef enum {
	EMMC_OK = 0,
	EMMC_ERROR = -1,
	EMMC_TIMEOUT = -2,
	EMMC_CRC_ERROR = -3,
	EMMC_BUSY = -4,
	EMMC_NOT_READY = -5,
	EMMC_INVALID_PARAM = -6,
	EMMC_NOT_SUPPORTED = -7,
	/* RPMB-specific error codes */
	EMMC_AUTH_ERROR = -8,
	EMMC_COUNTER_ERROR = -9,
	EMMC_ADDRESS_ERROR = -10,
	EMMC_READ_ERROR = -11,
	EMMC_WRITE_ERROR = -12,
	EMMC_PROTOCOL_ERROR = -13,
	EMMC_NONCE_ERROR = -14,
	EMMC_MAC_ERROR = -15
} emmc_result_t;

/* CID structure */
typedef struct {
	u8 mid;	   /* Manufacturer ID */
	u16 oid;   /* OEM/Application ID */
	u8 pnm[6]; /* Product name */
	u8 prv;	   /* Product revision */
	u32 psn;   /* Product serial number */
	u8 mdt;	   /* Manufacturing date */
	u8 crc;	   /* CRC7 checksum */
} emmc_cid_t;

typedef struct {
	u32 c_size;
	u8 c_size_mult;
	u8 read_bl_len;
} emmc_csd_t;

typedef struct {
	u8 ext_csd_rev;
	u8 card_type;
	u8 bus_width;
	u8 hs_timing;
	u8 strobe_support;
	u32 sec_count;
	u8 partition_config;
	u8 boot_mult;
	u8 rpmb_size_mult;
} emmc_ext_csd_t;

/* eMMC card info */
typedef struct {
	emmc_cid_t cid;
	emmc_csd_t csd;
	emmc_ext_csd_t ext_csd;
	u16 rca;
	u32 ocr;
	u64 capacity;
	emmc_state_t state;
	emmc_partition_t active_partition;
	emmc_bus_mode_t bus_mode;
	emmc_bus_width_t bus_width;
	u32 clock_freq;
	bool enhanced_strobe; /* Enhanced strobe enabled */
	bool initialized;
} emmc_card_info_t;

/* RPMB multi-frame support structures */
typedef struct {
	u16 frame_count;   /* Number of frames in buffer */
	u16 current_index; /* Current processing index */
	u8 *frame_buffer;  /* Pointer to frame buffer */
	u32 buffer_size;   /* Size of allocated buffer */
} emmc_rpmb_multi_frame_t;

/* RPMB batch operation context */
typedef struct {
	u16 start_address;		/* Starting block address */
	u16 total_blocks;		/* Total blocks to process */
	u16 completed_blocks;	/* Blocks already processed */
	bool is_read_operation; /* true for read, false for write */
	u8 *data_buffer;		/* User data buffer */
	u32 data_size;			/* Size of user data buffer */
} emmc_rpmb_batch_context_t;

/* Endian-specific types for RPMB documentation */
typedef u16 __be16; /* Big-endian 16-bit */
typedef u32 __be32; /* Big-endian 32-bit */

#endif /* EMMC_TYPES_H */
