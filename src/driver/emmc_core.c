#include "../include/emmc_config_presets.h" /* Include static configuration first */
#include "emmc_core.h"

/* Driver context (global state) */
static emmc_driver_context_t g_emmc_ctx = {0};

emmc_result_t emmc_driver_init(const emmc_driver_config_t *config)
{
	emmc_result_t result;

	if (!config) {
		return EMMC_INVALID_PARAM;
	}

	/* Initialize driver context */
	g_emmc_ctx.driver_config = *config;
	g_emmc_ctx.hal_config = config->hal_config;
	g_emmc_ctx.initialized = false;
	g_emmc_ctx.current_clock = 0;
	g_emmc_ctx.active_part = EMMC_PART_USER;

	/* Initialize HAL */
	result = hal_emmc_init(&config->hal_config);
	if (result != EMMC_OK) {
		return result;
	}

	/* Reset controller */
	result = hal_emmc_reset(HAL_EMMC_RESET_ALL);
	if (result != EMMC_OK) {
		return result;
	}

	/* Set initial power and clock */
	result = hal_emmc_set_power(true, HAL_EMMC_VOLTAGE_33V);
	if (result != EMMC_OK) {
		return result;
	}

	/* Set initial clock frequency */
	g_emmc_ctx.current_clock = hal_emmc_set_clock(HAL_EMMC_INIT_CLOCK_FREQ);
	if (g_emmc_ctx.current_clock == 0) {
		return EMMC_ERROR;
	}

	/* Enable clock */
	result = hal_emmc_clock_enable(true);
	if (result != EMMC_OK) {
		return result;
	}

	g_emmc_ctx.initialized = true;
	return EMMC_OK;
}

emmc_result_t emmc_driver_deinit(void)
{
	/* Disable clock */
	hal_emmc_clock_enable(false);

	/* Power down */
	hal_emmc_set_power(false, 0);

	/* Reset context */
	g_emmc_ctx.initialized = false;

	return EMMC_OK;
}

const emmc_driver_context_t *emmc_driver_get_context(void)
{
	return &g_emmc_ctx;
}

emmc_result_t emmc_send_command(u8 cmd_index, u32 argument,
								emmc_resp_type_t resp_type, u32 *response)
{
	hal_emmc_cmd_t cmd = {0};
	emmc_result_t result;

	if (!g_emmc_ctx.initialized) {
		return EMMC_NOT_READY;
	}

	/* Wait for controller to be ready */
	if (!hal_emmc_is_command_ready()) {
		result = hal_emmc_wait_cmd_ready(EMMC_CMD_TIMEOUT_MS);
		if (result != EMMC_OK) {
			return result;
		}
	}

	/* Setup command structure */
	cmd.index = cmd_index;
	cmd.argument = argument;
	cmd.resp_type = resp_type;
	cmd.data_present = false;
	cmd.crc_check = (resp_type != EMMC_RESP_NONE && resp_type != EMMC_RESP_R3);
	cmd.index_check = (resp_type != EMMC_RESP_NONE && resp_type != EMMC_RESP_R2);

	/* Send command */
	result = hal_emmc_send_command(&cmd, NULL);
	if (result != EMMC_OK) {
		return result;
	}

	/* Get response if requested */
	if (response && resp_type != EMMC_RESP_NONE) {
		result = hal_emmc_get_response(response, resp_type);
		if (result != EMMC_OK) {
			return result;
		}
	}

	return EMMC_OK;
}

emmc_result_t emmc_send_command_with_data(u8 cmd_index, u32 argument,
										  emmc_resp_type_t resp_type,
										  u8 *buffer, u32 block_size,
										  u32 block_count, bool read_operation,
										  u32 *response)
{
	hal_emmc_cmd_t cmd = {0};
	hal_emmc_data_t data = {0};
	emmc_result_t result;

	if (!g_emmc_ctx.initialized || !buffer) {
		return EMMC_INVALID_PARAM;
	}

	if (block_count > EMMC_MAX_BLOCK_COUNT) {
		return EMMC_INVALID_PARAM;
	}

	/* Wait for controller to be ready */
	if (!hal_emmc_is_command_ready() || hal_emmc_is_data_active()) {
		result = hal_emmc_wait_cmd_data_ready(EMMC_CMD_TIMEOUT_MS);
		if (result != EMMC_OK) {
			return result;
		}
	}

	/* Setup command structure */
	cmd.index = cmd_index;
	cmd.argument = argument;
	cmd.resp_type = resp_type;
	cmd.data_present = true;
	cmd.crc_check = (resp_type != EMMC_RESP_NONE && resp_type != EMMC_RESP_R3);
	cmd.index_check = (resp_type != EMMC_RESP_NONE && resp_type != EMMC_RESP_R2);

	/* Setup data structure */
	data.buffer = buffer;
	data.block_size = block_size;
	data.block_count = block_count;
	data.read_operation = read_operation;
	data.use_dma = g_emmc_ctx.hal_config.dma_enabled;

	/* Send command with data */
	result = hal_emmc_send_command(&cmd, &data);
	if (result != EMMC_OK) {
		return result;
	}

	/* Wait for data transfer completion */
	result = hal_emmc_wait_data_complete(EMMC_DATA_TIMEOUT_MS);
	if (result != EMMC_OK) {
		return result;
	}

	/* Get response if requested */
	if (response && resp_type != EMMC_RESP_NONE) {
		result = hal_emmc_get_response(response, resp_type);
		if (result != EMMC_OK) {
			return result;
		}
	}

	return EMMC_OK;
}

emmc_result_t emmc_get_card_status(u32 *status)
{
	if (!status || !g_emmc_ctx.initialized) {
		return EMMC_INVALID_PARAM;
	}

	return emmc_send_command(EMMC_CMD13,
							 (u32)g_emmc_ctx.card_info.rca << 16,
							 EMMC_RESP_R1, status);
}

bool emmc_is_card_ready(void)
{
	u32 status;
	emmc_result_t result;

	result = emmc_get_card_status(&status);
	if (result != EMMC_OK) {
		return false;
	}

	return (status & EMMC_R1_READY_FOR_DATA) != 0;
}

emmc_result_t emmc_card_initialize(void)
{
	emmc_result_t result;
	u32 response[4];
	u32 ocr = 0;
	u16 rca = 1;
	int retry_count;

	if (!g_emmc_ctx.initialized) {
		return EMMC_NOT_READY;
	}

	/* Reset card to idle state */
	result = emmc_send_command(EMMC_CMD0, 0, EMMC_RESP_NONE, NULL);
	if (result != EMMC_OK) {
		return result;
	}

	hal_emmc_delay_ms(10);

	/* Send operating conditions command with retry */
	for (retry_count = 0; retry_count < 100; retry_count++) {
		/* Check operating conditions */
		result = emmc_send_command(EMMC_CMD1, EMMC_OCR_VDD_33_34 | EMMC_OCR_SECTOR_MODE,
								   EMMC_RESP_R3, response);
		if (result != EMMC_OK) {
			return result;
		}

		ocr = response[0];

		/* Check if card is ready */
		if (ocr & EMMC_OCR_CARD_POWER_UP) {
			break;
		}

		hal_emmc_delay_ms(10);
	}

	if (!(ocr & EMMC_OCR_CARD_POWER_UP)) {
		return EMMC_TIMEOUT;
	}

	g_emmc_ctx.card_info.ocr = ocr;

	/* Get CID */
	result = emmc_send_command(EMMC_CMD2, 0, EMMC_RESP_R2, response);
	if (result != EMMC_OK) {
		return result;
	}

	if (EMMC_COMPILE_CID_PARSING) {
		result = emmc_parse_cid(response, &g_emmc_ctx.card_info.cid);
		if (result != EMMC_OK) {
			return result;
		}
	} else {
		/* Set minimal CID information */
		g_emmc_ctx.card_info.cid.manufacturer_id = 0x00;
		g_emmc_ctx.card_info.cid.product_serial = 0x00000000;
	}

	/* Set RCA */
	result = emmc_send_command(EMMC_CMD3, (u32)rca << 16, EMMC_RESP_R1, response);
	if (result != EMMC_OK) {
		return result;
	}

	g_emmc_ctx.card_info.rca = rca;

	/* Get CSD */
	result = emmc_send_command(EMMC_CMD9, (u32)rca << 16, EMMC_RESP_R2, response);
	if (result != EMMC_OK) {
		return result;
	}

	if (EMMC_COMPILE_CSD_PARSING) {
		result = emmc_parse_csd(response, &g_emmc_ctx.card_info.csd);
		if (result != EMMC_OK) {
			return result;
		}
	}

	/* Select card */
	result = emmc_send_command(EMMC_CMD7, (u32)rca << 16, EMMC_RESP_R1B, response);
	if (result != EMMC_OK) {
		return result;
	}

	/* Wait for card to be in transfer state */
	result = emmc_wait_for_state(EMMC_STATE_TRAN, 1000);
	if (result != EMMC_OK) {
		return result;
	}

	u8 ext_csd_buffer[512];
	result = emmc_send_command_with_data(EMMC_CMD8, 0, EMMC_RESP_R1,
										 ext_csd_buffer, 512, 1, true, response);
	if (result != EMMC_OK) {
		return result;
	}

	result = emmc_parse_ext_csd(ext_csd_buffer, &g_emmc_ctx.card_info.ext_csd);
	if (result != EMMC_OK) {
		return result;
	}

	g_emmc_ctx.card_info.capacity = emmc_calculate_capacity(&g_emmc_ctx.card_info.csd,
															&g_emmc_ctx.card_info.ext_csd);

	/* Set initial state */
	g_emmc_ctx.card_info.state = EMMC_STATE_TRAN;
	g_emmc_ctx.card_info.active_partition = EMMC_PART_USER;

	/* Set initial bus configuration */
#ifdef EMMC_STATIC_BUS_MODE
	g_emmc_ctx.card_info.bus_mode = EMMC_STATIC_BUS_MODE;
#else
	g_emmc_ctx.card_info.bus_mode = EMMC_MODE_SDR;
#endif

#ifdef EMMC_STATIC_BUS_WIDTH
	g_emmc_ctx.card_info.bus_width = EMMC_STATIC_BUS_WIDTH;
#else
	g_emmc_ctx.card_info.bus_width = EMMC_BUS_WIDTH_1;
#endif

	g_emmc_ctx.card_info.clock_freq = g_emmc_ctx.current_clock;

	if (EMMC_HAS_STATIC_BUS_MODE && (EMMC_STATIC_BUS_MODE == EMMC_MODE_HS400_ES)) {
		g_emmc_ctx.card_info.enhanced_strobe = true;
	} else {
		g_emmc_ctx.card_info.enhanced_strobe = false;
	}

	g_emmc_ctx.card_info.initialized = true;

	/* Apply static bus configuration immediately */
	if (EMMC_HAS_STATIC_BUS_MODE) {
		result = hal_emmc_set_timing(EMMC_STATIC_BUS_MODE);
		if (result != EMMC_OK) {
			return result;
		}
	}

	if (EMMC_HAS_STATIC_BUS_WIDTH) {
		result = hal_emmc_set_bus_width(EMMC_STATIC_BUS_WIDTH);
		if (result != EMMC_OK) {
			return result;
		}
	}

	return EMMC_OK;
}

emmc_result_t emmc_parse_cid(const u32 *response, emmc_cid_t *cid)
{
	if (!response || !cid) {
		return EMMC_INVALID_PARAM;
	}

	/* Parse CID register fields */
	cid->mid = (u8)((response[0] >> 24) & 0xFF);
	cid->oid = (u16)((response[0] >> 8) & 0xFFFF);

	cid->pnm[0] = (u8)(response[0] & 0xFF);
	cid->pnm[1] = (u8)((response[1] >> 24) & 0xFF);
	cid->pnm[2] = (u8)((response[1] >> 16) & 0xFF);
	cid->pnm[3] = (u8)((response[1] >> 8) & 0xFF);
	cid->pnm[4] = (u8)(response[1] & 0xFF);
	cid->pnm[5] = (u8)((response[2] >> 24) & 0xFF);

	cid->prv = (u8)((response[2] >> 16) & 0xFF);
	cid->psn = ((response[2] & 0xFFFF) << 16) | ((response[3] >> 16) & 0xFFFF);
	cid->mdt = (u8)((response[3] >> 8) & 0xFF);
	cid->crc = (u8)((response[3] >> 1) & 0x7F);

	return EMMC_OK;
}

emmc_result_t emmc_parse_csd(const u32 *response, emmc_csd_t *csd)
{
	if (!response || !csd) {
		return EMMC_INVALID_PARAM;
	}

	csd->read_bl_len = (u8)((response[1] >> 16) & 0xF);
	csd->c_size = ((response[1] & 0x3FF) << 2) | ((response[2] >> 30) & 0x3);
	csd->c_size_mult = (u8)((response[2] >> 15) & 0x7);

	return EMMC_OK;
}

emmc_result_t emmc_parse_ext_csd(const u8 *buffer, emmc_ext_csd_t *ext_csd)
{
	if (!buffer || !ext_csd) {
		return EMMC_INVALID_PARAM;
	}

	ext_csd->ext_csd_rev = buffer[192];
	ext_csd->card_type = buffer[196];
	ext_csd->bus_width = buffer[183];
	ext_csd->hs_timing = buffer[185];
	ext_csd->strobe_support = buffer[184];
	ext_csd->sec_count = (u32)buffer[212] |
						 ((u32)buffer[213] << 8) |
						 ((u32)buffer[214] << 16) |
						 ((u32)buffer[215] << 24);
	ext_csd->partition_config = buffer[179];
	ext_csd->boot_mult = buffer[226];
	ext_csd->rpmb_size_mult = buffer[168];

	return EMMC_OK;
}

u64 emmc_calculate_capacity(const emmc_csd_t *csd, const emmc_ext_csd_t *ext_csd)
{
	if (!csd || !ext_csd) {
		return 0;
	}

	/* For eMMC 4.4+, use SEC_COUNT from EXT_CSD if available */
	if (ext_csd->sec_count > 0) {
		return (u64)ext_csd->sec_count * 512;
	}

	/* Fallback to CSD calculation for older cards */
	u32 mult = 1 << (csd->c_size_mult + 2);
	u32 blocknr = (csd->c_size + 1) * mult;
	u32 block_len = 1 << csd->read_bl_len;

	return (u64)blocknr * block_len;
}

emmc_result_t emmc_check_card_status(u32 status)
{
	if (status & EMMC_R1_ERROR_MASK) {
		if (status & EMMC_R1_OUT_OF_RANGE)
			return EMMC_ERROR;
		if (status & EMMC_R1_ADDRESS_ERROR)
			return EMMC_ERROR;
		if (status & EMMC_R1_BLOCK_LEN_ERROR)
			return EMMC_ERROR;
		if (status & EMMC_R1_ERASE_SEQ_ERROR)
			return EMMC_ERROR;
		if (status & EMMC_R1_ERASE_PARAM)
			return EMMC_ERROR;
		if (status & EMMC_R1_WP_VIOLATION)
			return EMMC_ERROR;
		if (status & EMMC_R1_LOCK_UNLOCK_FAILED)
			return EMMC_ERROR;
		if (status & EMMC_R1_COM_CRC_ERROR)
			return EMMC_CRC_ERROR;
		if (status & EMMC_R1_ILLEGAL_COMMAND)
			return EMMC_ERROR;
		if (status & EMMC_R1_CARD_ECC_FAILED)
			return EMMC_ERROR;
		if (status & EMMC_R1_CC_ERROR)
			return EMMC_ERROR;
		if (status & EMMC_R1_ERROR)
			return EMMC_ERROR;
	}

	return EMMC_OK;
}

emmc_result_t emmc_wait_for_state(emmc_state_t target_state, u32 timeout_ms)
{
	u32 status;
	emmc_result_t result;
	u32 elapsed = 0;

	while (elapsed < timeout_ms) {
		result = emmc_get_card_status(&status);
		if (result != EMMC_OK) {
			return result;
		}

		if (EMMC_GET_CARD_STATE(status) == target_state) {
			return EMMC_OK;
		}

		hal_emmc_delay_ms(10);
		elapsed += 10;
	}

	return EMMC_TIMEOUT;
}

emmc_result_t emmc_read_cid(emmc_cid_t *cid)
{
	u32 response[4];
	emmc_result_t result;

	if (!cid || !g_emmc_ctx.initialized) {
		return EMMC_INVALID_PARAM;
	}

	result = emmc_send_command(EMMC_CMD2, 0, EMMC_RESP_R2, response);
	if (result != EMMC_OK) {
		return result;
	}

	return emmc_parse_cid(response, cid);
}

emmc_result_t emmc_read_csd(emmc_csd_t *csd)
{
	u32 response[4];
	emmc_result_t result;

	if (!csd || !g_emmc_ctx.initialized) {
		return EMMC_INVALID_PARAM;
	}

	result = emmc_send_command(EMMC_CMD9, g_emmc_ctx.card_info.rca << 16, EMMC_RESP_R2, response);
	if (result != EMMC_OK) {
		return result;
	}

	return emmc_parse_csd(response, csd);
}

emmc_result_t emmc_read_ext_csd(emmc_ext_csd_t *ext_csd)
{
	u8 buffer[512];
	emmc_result_t result;

	if (!ext_csd || !g_emmc_ctx.initialized) {
		return EMMC_INVALID_PARAM;
	}

	result = emmc_send_command_with_data(EMMC_CMD8, 0, EMMC_RESP_R1,
										 buffer, 512, 1, true, NULL);
	if (result != EMMC_OK) {
		return result;
	}

	return emmc_parse_ext_csd(buffer, ext_csd);
}

emmc_result_t emmc_switch_partition(emmc_partition_t partition)
{
	emmc_result_t result;
	u8 partition_config;

	if (!g_emmc_ctx.initialized) {
		return EMMC_NOT_READY;
	}

	if (partition > EMMC_PART_GP4) {
		return EMMC_INVALID_PARAM;
	}

	/* Read current partition configuration */
	partition_config = (partition & 0x07) << 3;

	/* Switch partition using SWITCH command */
	result = emmc_switch_mode(EMMC_SWITCH_MODE_WRITE_BYTE,
							  EXT_CSD_PARTITION_CONFIG,
							  partition_config,
							  EMMC_SWITCH_TIMEOUT_MS);
	if (result != EMMC_OK) {
		return result;
	}

	g_emmc_ctx.active_part = partition;
	return EMMC_OK;
}

emmc_result_t emmc_switch_mode(u8 access_mode, u8 index, u8 value, u32 timeout_ms)
{
	u32 argument;
	emmc_result_t result;

	if (!g_emmc_ctx.initialized) {
		return EMMC_NOT_READY;
	}

	/* Build SWITCH command argument */
	argument = (access_mode << 24) | (index << 16) | (value << 8);

	result = emmc_send_command(EMMC_CMD6, argument, EMMC_RESP_R1B, NULL);
	if (result != EMMC_OK) {
		return result;
	}

	/* Wait for card to be ready after switch */
	result = emmc_wait_for_state(EMMC_STATE_TRAN, timeout_ms);
	if (result != EMMC_OK) {
		return result;
	}

	return EMMC_OK;
}

emmc_result_t emmc_set_bus_width(emmc_bus_width_t width)
{
	if (EMMC_COMPILE_DYNAMIC_BUS_WIDTH) {
		emmc_result_t result;
		u8 bus_width_value;

		if (!g_emmc_ctx.initialized) {
			return EMMC_NOT_READY;
		}

		/* Map bus width enum to EXT_CSD value */
		switch (width) {
		case EMMC_BUS_WIDTH_1:
			bus_width_value = 0;
			break;
		case EMMC_BUS_WIDTH_4:
			bus_width_value = 1;
			break;
		case EMMC_BUS_WIDTH_8:
			bus_width_value = 2;
			break;
		default:
			return EMMC_INVALID_PARAM;
		}

		/* Set bus width in card */
		result = emmc_switch_mode(EMMC_SWITCH_MODE_WRITE_BYTE,
								  EXT_CSD_BUS_WIDTH,
								  bus_width_value,
								  EMMC_SWITCH_TIMEOUT_MS);
		if (result != EMMC_OK) {
			return result;
		}

		/* Set bus width in controller */
		result = hal_emmc_set_bus_width(width);
		if (result != EMMC_OK) {
			return result;
		}

		g_emmc_ctx.card_info.bus_width = width;
		return EMMC_OK;
	} else {
		/* Static bus width - no runtime change allowed */
		(void)width;
		return EMMC_NOT_SUPPORTED;
	}
}

emmc_result_t emmc_set_timing_mode(emmc_bus_mode_t mode)
{
	if (EMMC_COMPILE_DYNAMIC_BUS_MODE) {
		emmc_result_t result;
		u8 hs_timing_value;

		if (!g_emmc_ctx.initialized) {
			return EMMC_NOT_READY;
		}

		/* Map timing mode to EXT_CSD HS_TIMING value */
		switch (mode) {
		case EMMC_MODE_SDR:
			hs_timing_value = 0;
			break;
		case EMMC_MODE_HS200:
			hs_timing_value = 2;
			break;
		case EMMC_MODE_HS400:
		case EMMC_MODE_HS400_ES:
			hs_timing_value = 3;
			break;
		default:
			hs_timing_value = 1; /* High Speed */
			break;
		}

		/* Set timing mode in card */
		result = emmc_switch_mode(EMMC_SWITCH_MODE_WRITE_BYTE,
								  EXT_CSD_HS_TIMING,
								  hs_timing_value,
								  EMMC_SWITCH_TIMEOUT_MS);
		if (result != EMMC_OK) {
			return result;
		}

		/* Set timing mode in controller */
		result = hal_emmc_set_timing(mode);
		if (result != EMMC_OK) {
			return result;
		}

		g_emmc_ctx.card_info.bus_mode = mode;
		return EMMC_OK;
	} else {
		/* Static bus mode - no runtime change allowed */
		(void)mode;
		return EMMC_NOT_SUPPORTED;
	}
}

emmc_result_t emmc_set_block_length(u32 block_len)
{
	emmc_result_t result;

	if (!g_emmc_ctx.initialized) {
		return EMMC_NOT_READY;
	}

	if (block_len == 0 || block_len > 512) {
		return EMMC_INVALID_PARAM;
	}

	result = emmc_send_command(EMMC_CMD16, block_len, EMMC_RESP_R1, NULL);
	if (result != EMMC_OK) {
		return result;
	}

	return EMMC_OK;
}
