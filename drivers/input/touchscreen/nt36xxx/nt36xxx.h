// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2010 - 2018 Novatek, Inc.
 *
 * $Revision: 47247 $
 * $Date: 2019-07-10 10:41:36 +0800 (Wed, 10 Jul 2019) $
 *
 */

#ifndef		_LINUX_NVT_TOUCH_H
#define		_LINUX_NVT_TOUCH_H

#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/uaccess.h>
#include "nt36xxx_mem_map.h"
#include <linux/regulator/consumer.h>

// Xiaomi Panel specific
#ifdef CONFIG_MACH_LONGCHEER
#define XIAOMI_PANEL 1
#else
#define XIAOMI_PANEL 0
#endif

#ifdef CONFIG_TOUCHSCREEN_NVT_A
#define TOUCHSCREEN_TULIP 1
#else
#define TOUCHSCREEN_TULIP 0
#endif

#ifdef CONFIG_TOUCHSCREEN_NVT_E7S
#define TOUCHSCREEN_WHYRED 1
#else
#define TOUCHSCREEN_WHYRED 0
#endif

#ifdef CONFIG_TOUCHSCREEN_NVT_D2S
#define TOUCHSCREEN_WAYNE 1
#else
#define TOUCHSCREEN_WAYNE 0
#endif

#ifdef CONFIG_TOUCHSCREEN_NVT_F7A
#define TOUCHSCREEN_LAVENDER 1
#else
#define TOUCHSCREEN_LAVENDER 0
#endif

#if TOUCHSCREEN_TULIP || TOUCHSCREEN_LAVENDER
#define NVTTOUCH_RST_PIN 66
#define NVTTOUCH_INT_PIN 67
#else
#define NVT_TOUCH_RST_PIN 980
#define NVT_TOUCH_INT_PIN 943
#endif

#define INT_TRIGGER_TYPE IRQ_TYPE_EDGE_RISING

#define NVT_I2C_NAME "NVT-ts"
#define I2C_BLDR_Address 0x01
#define I2C_FW_Address 0x01
#define I2C_HW_Address 0x62
#define NVT_TS_NAME "NVTCapacitiveTouchScreen"

#define TOUCH_DEFAULT_MAX_WIDTH 1080
#if TOUCHSCREEN_WHYRED || TOUCHSCREEN_WAYNE
#define TOUCH_DEFAULT_MAX_HEIGHT 2160
#elif TOUCHSCREEN_TULIP
#define TOUCH_DEFAULT_MAX_HEIGHT 2280
#elif TOUCHSCREEN_LAVENDER
#define TOUCH_DEFAULT_MAX_HEIGHT 2340
#else
#define TOUCH_DEFAULT_MAX_HEIGHT 1920
#endif
#define TOUCH_MAX_FINGER_NUM 10
#define TOUCH_FORCE_NUM 1000

#define NVT_TOUCH_SUPPORT_HW_RST 0

#define WAKEUP_GESTURE 1
#if WAKEUP_GESTURE
extern const uint16_t gesture_key_array[];
#endif
#ifndef CONFIG_TOUCHSCREEN_NT36XXX_FW_UPDATE
#define BOOT_UPDATE_FIRMWARE 0
#else
#define BOOT_UPDATE_FIRMWARE 1
#if TOUCHSCREEN_TULIP
#define BOOT_UPDATE_FIRMWARE_NAME_TIANMA "novatek/tianma_nt36672a_miui_e7t.bin"
#define BOOT_UPDATE_FIRMWARE_NAME_SHENCHAO "novatek/shenchao_nt36672a_miui_e7t.bin"
#elif TOUCHSCREEN_WHYRED
#define BOOT_UPDATE_FIRMWARE_NAME "novatek/nt36672_miui_e7s.bin"
#define BOOT_UPDATE_FIRMWARE_NAME_TWO "novatek/hx_nt36672_miui_e7s.bin"
#elif TOUCHSCREEN_WAYNE
#define BOOT_UPDATE_FIRMWARE_NAME_TIANMA "novatek/tianma_nt36672_miui_d2s.bin"
#define BOOT_UPDATE_FIRMWARE_NAME_JDI "novatek/jdi_nt36672_miui_d2s.bin"
#elif TOUCHSCREEN_LAVENDER
#define BOOT_UPDATE_FIRMWARE_NAME_TIANMA "novatek/tianma_nt36672a_miui_f7a.bin"
#define BOOT_UPDATE_FIRMWARE_NAME_SHENCHAO "novatek/shenchao_nt36672a_miui_f7a.bin"
#else
#define BOOT_UPDATE_FIRMWARE_NAME "novatek_ts_fw.bin"
#endif
#endif

#define POINT_DATA_LEN 65

#if TOUCHSCREEN_LAVENDER
#define NVT_LOG(fmt, args...) pr_debug("[%s] %s %d: " fmt, NVT_I2C_NAME, __func__, __LINE__, ##args)
#endif

struct nvt_ts_data {
#if TOUCHSCREEN_LAVENDER
	uint8_t touch_state;
#endif
#if XIAOMI_PANEL
	struct work_struct nvt_work;
	struct regulator *vcc_i2c;
#endif
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct delayed_work nvt_fwu_work;
	uint16_t addr;
	int8_t phys[32];
	struct workqueue_struct *coord_workqueue;
	struct work_struct irq_work;
	struct notifier_block fb_notif;
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
	uint8_t xbuf[1025];
	struct mutex xbuf_lock;
	bool irq_enabled;
};

typedef enum {
	RESET_STATE_INIT = 0xA0,// IC reset
	RESET_STATE_REK,		// ReK baseline
	RESET_STATE_REK_FINISH,	// baseline is ready
	RESET_STATE_NORMAL_RUN,	// normal run
	RESET_STATE_MAX  = 0xAF
} RST_COMPLETE_STATE;

typedef enum {
    EVENT_MAP_HOST_CMD                      = 0x50,
    EVENT_MAP_HANDSHAKING_or_SUB_CMD_BYTE   = 0x51,
    EVENT_MAP_RESET_COMPLETE                = 0x60,
    EVENT_MAP_FWINFO                        = 0x78,
    EVENT_MAP_PROJECTID                     = 0x9A,
} I2C_EVENT_MAP;

#if TOUCHSCREEN_LAVENDER
extern char *saved_command_line;
#endif
extern struct nvt_ts_data *ts;
extern int32_t CTP_I2C_READ(struct i2c_client *client, uint16_t address, uint8_t *buf, uint16_t len);
extern int32_t CTP_I2C_WRITE(struct i2c_client *client, uint16_t address, uint8_t *buf, uint16_t len);
extern void nvt_bootloader_reset(void);
extern void nvt_sw_reset_idle(void);
extern int32_t nvt_check_fw_reset_state(RST_COMPLETE_STATE check_reset_state);
extern int32_t nvt_get_fw_info(void);
extern int32_t nvt_clear_fw_status(void);
extern int32_t nvt_check_fw_status(void);
extern int32_t nvt_set_page(uint16_t i2c_addr, uint32_t addr);
extern void nvt_stop_crc_reboot(void);

#endif /* _LINUX_NVT_TOUCH_H */
