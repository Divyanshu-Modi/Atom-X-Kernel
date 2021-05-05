/*
 * Copyright (C) 2010 - 2017 Novatek, Inc.
 * Copyright (C) 2019 XiaoMi, Inc.
 *
 * SPDX-License-Identifier: GPL-2.0 
 *
 * $Revision: 14061 $
 *
 * $Date: 2017-07-06 15:45:15 +0800 (Thursday, 06 July 2017) $
 */

#ifndef 	_LINUX_NVT_TOUCH_H
#define		_LINUX_NVT_TOUCH_H

#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/regulator/consumer.h>
#include "nt36xxx_mem_map.h"

#define NVTTOUCH_INT_PIN 943

#define INT_TRIGGER_TYPE IRQ_TYPE_EDGE_RISING

#define NVT_I2C_NAME "NVT-ts"
#define I2C_BLDR_Address 0x01
#define I2C_FW_Address 0x01
#define I2C_HW_Address 0x62

#define NVT_TS_NAME "NVTCapacitiveTouchScreen"
#define TOUCH_DEFAULT_MAX_WIDTH 1080
#define TOUCH_DEFAULT_MAX_HEIGHT 2160
#define TOUCH_MAX_FINGER_NUM 10
#define TOUCH_FORCE_NUM 1000

#define WAKEUP_GESTURE 1
#if WAKEUP_GESTURE
extern const uint16_t gesture_key_array[];
#endif

#ifndef CONFIG_BOOT_FIRMWARE_UPDATE_NVT_E7S
#define BOOT_UPDATE_FIRMWARE 0
#else
#define BOOT_UPDATE_FIRMWARE 1
#define BOOT_UPDATE_FIRMWARE_NAME "novatek/nt36672_miui_e7s.bin"
#define BOOT_UPDATE_FIRMWARE_NAME_TWO "novatek/hx_nt36672_miui_e7s.bin"
#endif

struct nvt_ts_data {
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct work_struct nvt_work;
	struct delayed_work nvt_fwu_work;
	uint16_t addr;
	int8_t phys[32];
	struct notifier_block fb_notif;
	struct regulator *vcc_i2c;
	uint8_t fw_ver;
	uint8_t x_num;
	uint8_t y_num;
	uint16_t abs_x_max;
	uint16_t abs_y_max;
	uint8_t max_touch_num;
	uint32_t int_trigger_type;
	int32_t irq_gpio;
	uint32_t irq_flags;
	int32_t reset_gpio;
	uint32_t reset_flags;
	struct mutex lock;
	const struct nvt_ts_mem_map *mmap;
	uint8_t carrier_system;
	uint16_t nvt_pid;
};

typedef enum {
	RESET_STATE_INIT = 0xA0,
	RESET_STATE_REK,
	RESET_STATE_REK_FINISH,
	RESET_STATE_NORMAL_RUN,
	RESET_STATE_MAX  = 0xAF
} RST_COMPLETE_STATE;

typedef enum {
    EVENT_MAP_HOST_CMD                      = 0x50,
    EVENT_MAP_HANDSHAKING_or_SUB_CMD_BYTE   = 0x51,
    EVENT_MAP_RESET_COMPLETE                = 0x60,
    EVENT_MAP_FWINFO                        = 0x78,
    EVENT_MAP_PROJECTID                     = 0x9A,
} I2C_EVENT_MAP;

extern struct nvt_ts_data *ts;
extern int32_t CTP_I2C_READ(struct i2c_client *client, uint16_t address, uint8_t *buf, uint16_t len);
extern int32_t CTP_I2C_WRITE(struct i2c_client *client, uint16_t address, uint8_t *buf, uint16_t len);
extern void nvt_bootloader_reset(void);
extern void nvt_sw_reset_idle(void);
extern int32_t nvt_check_fw_reset_state(RST_COMPLETE_STATE check_reset_state);
extern int32_t nvt_get_fw_info(void);
extern int32_t nvt_clear_fw_status(void);
extern int32_t nvt_check_fw_status(void);
#endif /* _LINUX_NVT_TOUCH_H */
