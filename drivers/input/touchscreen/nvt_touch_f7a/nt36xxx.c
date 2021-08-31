/*
 * Copyright (C) 2010 - 2017 Novatek, Inc.
 * Copyright (C) 2019 XiaoMi, Inc.
 *
 * SPDX-License-Identifier: GPL-2.0
 * 
 * $Revision: 23175 $
 *
 * $Date: 2018-02-12 16:26:21 +0800 (Monday, 12 February 2018) $
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <asm/uaccess.h>
#include <linux/input/mt.h>
#include <linux/wakelock.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <linux/notifier.h>
#include <linux/fb.h>
#include "nt36xxx.h"

extern char g_lcd_id[128];
static struct work_struct g_resume_work;

struct g_nvt_data {
	bool valid;
	bool usb_plugin;
	struct work_struct nvt_usb_plugin_work;
};

struct g_nvt_data g_nvt = {0};
EXPORT_SYMBOL(g_nvt);

#define POINT_DATA_LEN 65

struct nvt_ts_data *ts;
static uint8_t bTouchIsAwake;

#if BOOT_UPDATE_FIRMWARE
static struct workqueue_struct *nvt_fwu_wq;
extern void Boot_Update_Firmware(struct work_struct *work);
#endif
static void do_nvt_ts_resume_work(struct work_struct *work);
static int fb_notifier_callback(struct notifier_block *self, unsigned long event, void *data);

#if WAKEUP_GESTURE
#ifdef CONFIG_TOUCHSCREEN_COMMON
#include <linux/input/tp_common.h>
#endif
#define WAKEUP_OFF 4
#define WAKEUP_ON 5
#define GESTURE_WORD_C          12
#define GESTURE_WORD_W          13
#define GESTURE_WORD_V          14
#define GESTURE_DOUBLE_CLICK    15
#define GESTURE_WORD_Z          16
#define GESTURE_WORD_M          17
#define GESTURE_WORD_O          18
#define GESTURE_WORD_e          19
#define GESTURE_WORD_S          20
#define GESTURE_SLIDE_UP        21
#define GESTURE_SLIDE_DOWN      22
#define GESTURE_SLIDE_LEFT      23
#define GESTURE_SLIDE_RIGHT     24
#define DATA_PROTOCOL           30
#define FUNCPAGE_GESTURE         1

const uint16_t gesture_key_array[] = {
	KEY_WAKEUP,
	KEY_WAKEUP,
	KEY_WAKEUP,
	KEY_WAKEUP,
	KEY_WAKEUP,
	KEY_WAKEUP,
	KEY_WAKEUP,
	KEY_WAKEUP,
	KEY_WAKEUP,
	KEY_WAKEUP,
	KEY_WAKEUP,
	KEY_WAKEUP,
	KEY_WAKEUP,
};

bool enable_gesture_mode = false;
EXPORT_SYMBOL(enable_gesture_mode);
bool delay_gesture = false;
bool suspend_state = false;
static struct wake_lock gestrue_wakelock;

int nvt_gesture_switch(struct input_dev *dev, unsigned int type, unsigned int code, int value)
{
	if (type == EV_SYN && code == SYN_CONFIG) {
		if (suspend_state) {
			if ((value != WAKEUP_OFF) || enable_gesture_mode) {
				delay_gesture = true;
			}
		}

		if (value == WAKEUP_OFF) {
			enable_gesture_mode = false;
		} else if (value == WAKEUP_ON) {
			enable_gesture_mode  = true;
		}
	}

	return 0;
}

#ifdef CONFIG_TOUCHSCREEN_COMMON
static ssize_t double_tap_show(struct kobject *kobj,
			       struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", enable_gesture_mode);
}

static ssize_t double_tap_store(struct kobject *kobj,
				struct kobj_attribute *attr, const char *buf,
				size_t count)
{
	int rc, val;

	rc = kstrtoint(buf, 10, &val);
	if (rc)
		return -EINVAL;

	enable_gesture_mode = !!val;
	return count;
}

static struct tp_common_ops double_tap_ops = {
	.show = double_tap_show,
	.store = double_tap_store
};
#endif

void nvt_ts_wakeup_gesture_report(uint8_t gesture_id, uint8_t *data)
{
	uint32_t keycode = 0;
	uint8_t func_type = data[2];
	uint8_t func_id = data[3];

	if ((gesture_id == DATA_PROTOCOL) && (func_type == FUNCPAGE_GESTURE))
		gesture_id = func_id;
	else if (gesture_id > DATA_PROTOCOL)
		return;

	switch (gesture_id) {
		case GESTURE_WORD_C:
			keycode = gesture_key_array[0];
			break;
		case GESTURE_WORD_W:
			keycode = gesture_key_array[1];
			break;
		case GESTURE_WORD_V:
			keycode = gesture_key_array[2];
			break;
		case GESTURE_DOUBLE_CLICK:
			keycode = gesture_key_array[3];
			break;
		case GESTURE_WORD_Z:
			keycode = gesture_key_array[4];
			break;
		case GESTURE_WORD_M:
			keycode = gesture_key_array[5];
			break;
		case GESTURE_WORD_O:
			keycode = gesture_key_array[6];
			break;
		case GESTURE_WORD_e:
			keycode = gesture_key_array[7];
			break;
		case GESTURE_WORD_S:
			keycode = gesture_key_array[8];
			break;
		case GESTURE_SLIDE_UP:
			keycode = gesture_key_array[9];
			break;
		case GESTURE_SLIDE_DOWN:
			keycode = gesture_key_array[10];
			break;
		case GESTURE_SLIDE_LEFT:
			keycode = gesture_key_array[11];
			break;
		case GESTURE_SLIDE_RIGHT:
			keycode = gesture_key_array[12];
			break;
		default:
			break;
	}

	if (keycode > 0) {
		input_report_key(ts->input_dev, keycode, 1);
		input_sync(ts->input_dev);
		input_report_key(ts->input_dev, keycode, 0);
		input_sync(ts->input_dev);
	}
}
#endif

int32_t CTP_I2C_READ(struct i2c_client *client, uint16_t address, uint8_t *buf, uint16_t len)
{
	struct i2c_msg msgs[2];
	int32_t ret = -1;
	uint8_t retries;

	msgs[0].flags = !I2C_M_RD;
	msgs[0].addr  = address;
	msgs[0].len   = 1;
	msgs[0].buf   = &buf[0];

	msgs[1].flags = I2C_M_RD;
	msgs[1].addr  = address;
	msgs[1].len   = len - 1;
	msgs[1].buf   = &buf[1];

	for (retries = 0; retries < 5; retries++) {
		ret = i2c_transfer(client->adapter, msgs, 2);
		if (ret == 2)
			return ret;
	}

	return -EIO;
}

int32_t CTP_I2C_WRITE(struct i2c_client *client, uint16_t address, uint8_t *buf, uint16_t len)
{
	struct i2c_msg msg;
	int32_t ret = -1;
	uint8_t retries;

	msg.flags = !I2C_M_RD;
	msg.addr  = address;
	msg.len   = len;
	msg.buf   = buf;

	for (retries = 0; retries < 5; retries++) {
		ret = i2c_transfer(client->adapter, &msg, 1);
		if (ret == 1)
			return ret;
	}

	return -EIO;
}


void nvt_sw_reset_idle(void)
{
	uint8_t buf[4]={0};

	buf[0]=0x00;
	buf[1]=0xA5;
	CTP_I2C_WRITE(ts->client, I2C_HW_Address, buf, 2);

	msleep(15);
}

void nvt_bootloader_reset(void)
{
	uint8_t buf[8] = {0};

	buf[0] = 0x00;
	buf[1] = 0x69;
	CTP_I2C_WRITE(ts->client, I2C_HW_Address, buf, 2);

	msleep(35);
}

int32_t nvt_clear_fw_status(void)
{
	uint8_t buf[8] = {0};
	int32_t i = 0;
	const int32_t retry = 20;

	for (i = 0; i < retry; i++) {
		buf[0] = 0xFF;
		buf[1] = (ts->mmap->EVENT_BUF_ADDR >> 16) & 0xFF;
		buf[2] = (ts->mmap->EVENT_BUF_ADDR >> 8) & 0xFF;
		CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, 3);

		buf[0] = EVENT_MAP_HANDSHAKING_or_SUB_CMD_BYTE;
		buf[1] = 0x00;
		CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, 2);

		buf[0] = EVENT_MAP_HANDSHAKING_or_SUB_CMD_BYTE;
		buf[1] = 0xFF;
		CTP_I2C_READ(ts->client, I2C_FW_Address, buf, 2);

		if (buf[1] == 0x00)
			break;

		msleep(10);
	}

	if (i >= retry)
		return -1;
	else
		return 0;
}

int32_t nvt_check_fw_status(void)
{
	uint8_t buf[8] = {0};
	int32_t i = 0;
	const int32_t retry = 50;

	for (i = 0; i < retry; i++) {
		buf[0] = 0xFF;
		buf[1] = (ts->mmap->EVENT_BUF_ADDR >> 16) & 0xFF;
		buf[2] = (ts->mmap->EVENT_BUF_ADDR >> 8) & 0xFF;
		CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, 3);

		buf[0] = EVENT_MAP_HANDSHAKING_or_SUB_CMD_BYTE;
		buf[1] = 0x00;
		CTP_I2C_READ(ts->client, I2C_FW_Address, buf, 2);

		if ((buf[1] & 0xF0) == 0xA0)
			break;

		msleep(10);
	}

	if (i >= retry)
		return -1;
	else
		return 0;
}

int32_t nvt_check_fw_reset_state(RST_COMPLETE_STATE check_reset_state)
{
	uint8_t buf[8] = {0};
	int32_t ret = 0;
	int32_t retry = 0;

	while (1) {
		msleep(10);

		buf[0] = EVENT_MAP_RESET_COMPLETE;
		buf[1] = 0x00;
		CTP_I2C_READ(ts->client, I2C_FW_Address, buf, 6);

		if ((buf[1] >= check_reset_state) && (buf[1] <= RESET_STATE_MAX)) {
			ret = 0;
			break;
		}

		retry++;
		if (unlikely(retry > 100)) {
			ret = -1;
			break;
		}
	}

	return ret;
}

int32_t nvt_read_pid(void)
{
	uint8_t buf[3] = {0};
	int32_t ret = 0;

	buf[0] = 0xFF;
	buf[1] = (ts->mmap->EVENT_BUF_ADDR >> 16) & 0xFF;
	buf[2] = (ts->mmap->EVENT_BUF_ADDR >> 8) & 0xFF;
	CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, 3);

	buf[0] = EVENT_MAP_PROJECTID;
	buf[1] = 0x00;
	buf[2] = 0x00;
	CTP_I2C_READ(ts->client, I2C_FW_Address, buf, 3);

	ts->nvt_pid = (buf[2] << 8) + buf[1];

	return ret;
}

int32_t nvt_get_fw_info(void)
{
	uint8_t buf[64] = {0};
	uint32_t retry_count = 0;
	int32_t ret = 0;

info_retry:
	buf[0] = 0xFF;
	buf[1] = (ts->mmap->EVENT_BUF_ADDR >> 16) & 0xFF;
	buf[2] = (ts->mmap->EVENT_BUF_ADDR >> 8) & 0xFF;
	CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, 3);

	buf[0] = EVENT_MAP_FWINFO;
	CTP_I2C_READ(ts->client, I2C_FW_Address, buf, 17);
	ts->x_num = buf[3];
	ts->y_num = buf[4];
	ts->abs_x_max = (uint16_t)((buf[5] << 8) | buf[6]);
	ts->abs_y_max = (uint16_t)((buf[7] << 8) | buf[8]);

	if ((buf[1] + buf[2]) != 0xFF) {
		ts->x_num = 18;
		ts->y_num = 32;
		ts->abs_x_max = TOUCH_DEFAULT_MAX_WIDTH;
		ts->abs_y_max = TOUCH_DEFAULT_MAX_HEIGHT;

		if (retry_count < 3) {
			retry_count++;
			goto info_retry;
		} else
			ret = -1;
	} else
		ret = 0;

	nvt_read_pid();
	return ret;
}

#ifdef CONFIG_OF
static void nvt_parse_dt(struct device *dev)
{
	struct device_node *np = dev->of_node;

#if NVT_TOUCH_SUPPORT_HW_RST
	ts->reset_gpio = of_get_named_gpio_flags(np, "novatek,reset-gpio", 0, &ts->reset_flags);
#endif
	ts->irq_gpio = of_get_named_gpio_flags(np, "novatek,irq-gpio", 0, &ts->irq_flags);
}
#else
static void nvt_parse_dt(struct device *dev)
{
#if NVT_TOUCH_SUPPORT_HW_RST
	ts->reset_gpio = NVTTOUCH_RST_PIN;
#endif
	ts->irq_gpio = NVTTOUCH_INT_PIN;
}
#endif

static int nvt_gpio_config(struct nvt_ts_data *ts)
{
	int32_t ret = 0;

#if NVT_TOUCH_SUPPORT_HW_RST
	if (gpio_is_valid(ts->reset_gpio)) {
		ret = gpio_request_one(ts->reset_gpio, GPIOF_OUT_INIT_HIGH, "NVT-tp-rst");
		if (ret)
			goto err_request_reset_gpio;
	}
#endif

	if (gpio_is_valid(ts->irq_gpio)) {
		ret = gpio_request_one(ts->irq_gpio, GPIOF_IN, "NVT-int");
		if (ret)
			goto err_request_irq_gpio;
	}

	return ret;

err_request_irq_gpio:
#if NVT_TOUCH_SUPPORT_HW_RST
	gpio_free(ts->reset_gpio);
err_request_reset_gpio:
#endif

	return ret;
}

static void nvt_ts_work_func(void)
{
	int32_t ret = -1;
	uint8_t point_data[POINT_DATA_LEN + 1] = {0};
	uint32_t position = 0;
	uint32_t input_x = 0;
	uint32_t input_y = 0;
	uint8_t input_id = 0;
	uint8_t press_id[TOUCH_MAX_FINGER_NUM] = {0};
	int32_t i = 0;
	int32_t finger_cnt = 0;
	struct sched_param param = { .sched_priority = MAX_USER_RT_PRIO / 2 };

	sched_setscheduler(current, SCHED_FIFO, &param);

	ret = CTP_I2C_READ(ts->client, I2C_FW_Address, point_data, POINT_DATA_LEN + 1);
	if (unlikely(ret < 0)) {
		return;
	}

#if WAKEUP_GESTURE
	if (bTouchIsAwake == 0) {
		input_id = (uint8_t) (point_data[1] >> 3);
		nvt_ts_wakeup_gesture_report(input_id, point_data);
		enable_irq(ts->client->irq);
		mutex_unlock(&ts->lock);
		return;
	}
#endif

	for (i = 0; i < ts->max_touch_num; i++) {
		position = 1 + 6 * i;
		input_id = (uint8_t) (point_data[position] >> 3);
		if ((input_id == 0) || (input_id > ts->max_touch_num))
			continue;

		if (likely(((point_data[position] & 0x07) == 0x01) || ((point_data[position] & 0x07) == 0x02))) {
			input_x = (uint32_t) (point_data[position + 1] << 4) + (uint32_t) (point_data[position + 3] >> 4);
			input_y = (uint32_t) (point_data[position + 2] << 4) + (uint32_t) (point_data[position + 3] & 0x0F);
			press_id[input_id - 1] = 1;
			input_mt_slot(ts->input_dev, input_id - 1);
			input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, true);
			input_report_abs(ts->input_dev, ABS_MT_POSITION_X, input_x);
			input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, input_y);
			input_report_abs(ts->input_dev, ABS_MT_PRESSURE, TOUCH_FORCE_NUM);
			finger_cnt++;
		}
	}

	for (i = 0; i < ts->max_touch_num; i++) {
		if (likely(press_id[i] != 1)) {
			input_mt_slot(ts->input_dev, i);
			input_report_abs(ts->input_dev, ABS_MT_PRESSURE, 0);
			input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, false);
		}
	}

	input_report_key(ts->input_dev, BTN_TOUCH, (finger_cnt > 0));
	input_sync(ts->input_dev);
}

static void nvt_ts_usb_plugin_work_func(struct work_struct *work)
{
	uint8_t buf[8] = {0};
	int32_t ret = 0;

	if ( !bTouchIsAwake || (ts->touch_state != TOUCH_STATE_WORKING) ) {
		return;
	}

	mutex_lock(&ts->lock);

	msleep(35);

	buf[0] = 0xFF;
	buf[1] = (ts->mmap->EVENT_BUF_ADDR >> 16) & 0xFF;
	buf[2] = (ts->mmap->EVENT_BUF_ADDR >> 8) & 0xFF;
	ret = CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, 3);
	if (ret < 0)
		goto exit;

	buf[0] = EVENT_MAP_HOST_CMD;
	if (g_nvt.usb_plugin)
		buf[1] = 0x53;
	else
		buf[1] = 0x51;

	ret = CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, 2);
	if (ret < 0)
		goto exit;

exit:
	mutex_unlock(&ts->lock);
}

static irqreturn_t nvt_ts_irq_handler(int32_t irq, void *dev_id)
{
#if WAKEUP_GESTURE
	if (bTouchIsAwake == 0) {
		wake_lock_timeout(&gestrue_wakelock, msecs_to_jiffies(5000));
	}
#endif

	mutex_lock(&ts->lock);
	nvt_ts_work_func();
	mutex_unlock(&ts->lock);

	return IRQ_HANDLED;
}

void nvt_stop_crc_reboot(void)
{
	uint8_t buf[8] = {0};
	int32_t retry = 0;

	buf[0] = 0xFF;
	buf[1] = 0x01;
	buf[2] = 0xF6;
	CTP_I2C_WRITE(ts->client, I2C_BLDR_Address, buf, 3);

	buf[0] = 0x4E;
	CTP_I2C_READ(ts->client, I2C_BLDR_Address, buf, 4);

	if ((buf[1] == 0xFC) ||
			((buf[1] == 0xFF) && (buf[2] == 0xFF) && (buf[3] == 0xFF))) {

		for (retry = 5; retry > 0; retry--) {
			buf[0]=0x00;
			buf[1]=0xA5;
			CTP_I2C_WRITE(ts->client, I2C_HW_Address, buf, 2);

			buf[0]=0x00;
			buf[1]=0xA5;
			CTP_I2C_WRITE(ts->client, I2C_HW_Address, buf, 2);
			msleep(1);

			buf[0] = 0xFF;
			buf[1] = 0x03;
			buf[2] = 0xF1;
			CTP_I2C_WRITE(ts->client, I2C_BLDR_Address, buf, 3);

			buf[0] = 0x35;
			buf[1] = 0xA5;
			CTP_I2C_WRITE(ts->client, I2C_BLDR_Address, buf, 2);

			buf[0] = 0xFF;
			buf[1] = 0x03;
			buf[2] = 0xF1;
			CTP_I2C_WRITE(ts->client, I2C_BLDR_Address, buf, 3);

			buf[0] = 0x35;
			buf[1] = 0x00;
			CTP_I2C_READ(ts->client, I2C_BLDR_Address, buf, 2);

			if (buf[1] == 0xA5)
				break;
		}
	}

	return;
}

static int8_t nvt_ts_check_chip_ver_trim(void)
{
	uint8_t buf[8] = {0};
	int32_t retry = 0;
	int32_t list = 0;
	int32_t i = 0;
	int32_t found_nvt_chip = 0;
	int32_t ret = -1;

	nvt_bootloader_reset();

	for (retry = 5; retry > 0; retry--) {
		nvt_sw_reset_idle();

		buf[0] = 0x00;
		buf[1] = 0x35;
		CTP_I2C_WRITE(ts->client, I2C_HW_Address, buf, 2);
		msleep(10);

		buf[0] = 0xFF;
		buf[1] = 0x01;
		buf[2] = 0xF6;
		CTP_I2C_WRITE(ts->client, I2C_BLDR_Address, buf, 3);

		buf[0] = 0x4E;
		buf[1] = 0x00;
		buf[2] = 0x00;
		buf[3] = 0x00;
		buf[4] = 0x00;
		buf[5] = 0x00;
		buf[6] = 0x00;
		CTP_I2C_READ(ts->client, I2C_BLDR_Address, buf, 7);

		if ((buf[1] == 0xFC) ||
				((buf[1] == 0xFF) && (buf[2] == 0xFF) && (buf[3] == 0xFF))) {
			nvt_stop_crc_reboot();
			continue;
		}

		for (list = 0; list < (sizeof(trim_id_table) / sizeof(struct nvt_ts_trim_id_table)); list++) {
			found_nvt_chip = 0;

			for (i = 0; i < NVT_ID_BYTE_MAX; i++)
				if (trim_id_table[list].mask[i])
					if (buf[i + 1] != trim_id_table[list].id[i])
						break;

			if (i == NVT_ID_BYTE_MAX)
				found_nvt_chip = 1;

			if (found_nvt_chip) {
				ts->mmap = trim_id_table[list].mmap;
				ts->carrier_system = trim_id_table[list].carrier_system;
				ret = 0;
				goto out;
			} else {
				ts->mmap = NULL;
				ret = -1;
			}
		}

		msleep(10);
	}

out:
	return ret;
}

static int32_t nvt_ts_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int32_t ret = 0;
#if WAKEUP_GESTURE
	int32_t retry = 0;
#endif

	ts = kzalloc(sizeof(struct nvt_ts_data), GFP_KERNEL);
	if (IS_ERR_OR_NULL(ts)) {
		return -ENOMEM;
	}

	ts->client = client;
	i2c_set_clientdata(client, ts);
	nvt_parse_dt(&client->dev);

	ret = nvt_gpio_config(ts);
	if (ret) {
		goto err_gpio_config_failed;
	}

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		ret = -ENODEV;
		goto err_check_functionality_failed;
	}

	msleep(10);

	ret = nvt_ts_check_chip_ver_trim();
	if (ret) {
		ret = -EINVAL;
		goto err_chipvertrim_failed;
	}

	mutex_init(&ts->lock);
	mutex_init(&ts->pm_mutex);
	mutex_lock(&ts->lock);
	nvt_bootloader_reset();
	nvt_check_fw_reset_state(RESET_STATE_INIT);
	nvt_get_fw_info();
	mutex_unlock(&ts->lock);

	INIT_WORK(&g_nvt.nvt_usb_plugin_work, nvt_ts_usb_plugin_work_func);

	ts->input_dev = input_allocate_device();
	if (ts->input_dev == NULL) {
		ret = -ENOMEM;
		goto err_input_dev_alloc_failed;
	}

	ts->max_touch_num = TOUCH_MAX_FINGER_NUM;
	ts->int_trigger_type = INT_TRIGGER_TYPE;
	ts->input_dev->evbit[0] = BIT_MASK(EV_SYN) | BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS) ;
	ts->input_dev->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);
	ts->input_dev->propbit[0] = BIT(INPUT_PROP_DIRECT);

	input_mt_init_slots(ts->input_dev, ts->max_touch_num, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_PRESSURE, 0, TOUCH_FORCE_NUM, 0, 0);

#if TOUCH_MAX_FINGER_NUM > 1
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_X, 0, ts->abs_x_max-1, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_Y, 0, ts->abs_y_max-1, 0, 0);
#endif

#if WAKEUP_GESTURE
	ts->input_dev->event =nvt_gesture_switch;
	for (retry = 0; retry < (sizeof(gesture_key_array) / sizeof(gesture_key_array[0])); retry++) {
		input_set_capability(ts->input_dev, EV_KEY, gesture_key_array[retry]);
	}
	wake_lock_init(&gestrue_wakelock, WAKE_LOCK_SUSPEND, "poll-wake-lock");
#ifdef CONFIG_TOUCHSCREEN_COMMON
	ret = tp_common_set_double_tap_ops(&double_tap_ops);
#endif
#endif

	sprintf(ts->phys, "input/ts");
	ts->input_dev->name = NVT_TS_NAME;
	ts->input_dev->phys = ts->phys;
	ts->input_dev->id.bustype = BUS_I2C;

	ret = input_register_device(ts->input_dev);
	if (ret)
		goto err_input_register_device_failed;

	client->irq = gpio_to_irq(ts->irq_gpio);
	if (client->irq) {
#if WAKEUP_GESTURE
		ret = request_threaded_irq(client->irq, NULL, nvt_ts_irq_handler, ts->int_trigger_type | IRQF_ONESHOT | IRQF_NO_SUSPEND, client->name, ts);
#else
		ret = request_threaded_irq(client->irq, NULL, nvt_ts_irq_handler, ts->int_trigger_type | IRQF_ONESHOT, client->name, ts);
#endif
		if (ret != 0)
			goto err_int_request_failed;
		else
			disable_irq(client->irq);
	}

#if BOOT_UPDATE_FIRMWARE
	nvt_fwu_wq = create_singlethread_workqueue("nvt_fwu_wq");
	if (!nvt_fwu_wq) {
		ret = -ENOMEM;
		goto err_create_nvt_fwu_wq_failed;
	}
	INIT_DELAYED_WORK(&ts->nvt_fwu_work, Boot_Update_Firmware);

	queue_delayed_work(nvt_fwu_wq, &ts->nvt_fwu_work, msecs_to_jiffies(14000));
#endif

	ts->fb_notif.notifier_call = fb_notifier_callback;
	ret = fb_register_client(&ts->fb_notif);
	if(ret)
		goto err_register_fb_notif_failed;

	INIT_WORK(&g_resume_work, do_nvt_ts_resume_work);

	bTouchIsAwake = 1;
	enable_irq(client->irq);

	g_nvt.valid = true;
	g_nvt.usb_plugin = false;

	return 0;

err_register_fb_notif_failed:
	free_irq(client->irq, ts);
#if BOOT_UPDATE_FIRMWARE
err_create_nvt_fwu_wq_failed:
#endif
err_int_request_failed:
err_input_register_device_failed:
	input_free_device(ts->input_dev);
err_input_dev_alloc_failed:
err_chipvertrim_failed:
err_check_functionality_failed:
	gpio_free(ts->irq_gpio);
err_gpio_config_failed:
	i2c_set_clientdata(client, NULL);
	kfree(ts);
	return ret;
}

static int32_t nvt_ts_remove(struct i2c_client *client)
{
	fb_unregister_client(&ts->fb_notif);
	cancel_work_sync(&g_resume_work);
	mutex_destroy(&ts->lock);
	mutex_destroy(&ts->pm_mutex);
	free_irq(client->irq, ts);
	input_unregister_device(ts->input_dev);
	i2c_set_clientdata(client, NULL);
	kfree(ts);

	return 0;
}

static int32_t nvt_ts_suspend(struct device *dev)
{
	uint8_t buf[4] = {0};
	uint32_t i = 0;

	if (!bTouchIsAwake)
		return 0;

	mutex_lock(&ts->lock);
	bTouchIsAwake = 0;

#if WAKEUP_GESTURE
	if (enable_gesture_mode) {
		buf[0] = EVENT_MAP_HOST_CMD;
		buf[1] = 0x13;
		CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, 2);
		enable_irq_wake(ts->client->irq);
	} else {
#endif
		disable_irq(ts->client->irq);
		buf[0] = EVENT_MAP_HOST_CMD;
		buf[1] = 0x11;
		CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, 2);
#if WAKEUP_GESTURE
	}
#endif

	for (i = 0; i < ts->max_touch_num; i++) {
		input_mt_slot(ts->input_dev, i);
		input_report_abs(ts->input_dev, ABS_MT_PRESSURE, 0);
		input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, 0);
	}
	input_report_key(ts->input_dev, BTN_TOUCH, 0);
	input_sync(ts->input_dev);

	msleep(50);
	mutex_unlock(&ts->lock);
#if WAKEUP_GESTURE
	suspend_state = true;
#endif

	return 0;
}

static int32_t nvt_ts_resume(struct device *dev)
{
	if (bTouchIsAwake)
		return 0;

	mutex_lock(&ts->lock);
#if NVT_TOUCH_SUPPORT_HW_RST
	gpio_set_value(ts->reset_gpio, 1);
#endif
	nvt_bootloader_reset();
	nvt_check_fw_reset_state(RESET_STATE_REK);

#if WAKEUP_GESTURE
	if (delay_gesture)
		enable_gesture_mode = !enable_gesture_mode;

	if (!enable_gesture_mode)
		enable_irq(ts->client->irq);

	if (delay_gesture)
		enable_gesture_mode = !enable_gesture_mode;
#else
	enable_irq(ts->client->irq);
#endif

	bTouchIsAwake = 1;
	mutex_unlock(&ts->lock);
#if WAKEUP_GESTURE
	suspend_state = false;
	delay_gesture = false;
#endif

	if (g_nvt.usb_plugin)
		nvt_ts_usb_plugin_work_func(NULL);

	return 0;
}

static void do_nvt_ts_resume_work(struct work_struct *work)
{
	int ret = 0;
	mutex_lock(&ts->pm_mutex);
	ret = nvt_ts_resume(&ts->client->dev);
	mutex_unlock(&ts->pm_mutex);
	return;
}

static int fb_notifier_callback(struct notifier_block *self, unsigned long event, void *data)
{
	struct fb_event *evdata = data;
	int *blank;
	struct nvt_ts_data *ts =
		container_of(self, struct nvt_ts_data, fb_notif);

	if (evdata && evdata->data && event == FB_EARLY_EVENT_BLANK) {
		blank = evdata->data;
		if (*blank == FB_BLANK_POWERDOWN)
			flush_work(&g_resume_work);
			mutex_lock(&ts->pm_mutex);
			nvt_ts_suspend(&ts->client->dev);
			mutex_unlock(&ts->pm_mutex);
	} else if (evdata && evdata->data && event == FB_EVENT_BLANK) {
		blank = evdata->data;
		if (*blank == FB_BLANK_UNBLANK)
			schedule_work(&g_resume_work);
	}

	return 0;
}

static const struct i2c_device_id nvt_ts_id[] = {
	{ NVT_I2C_NAME, 0 },
	{ }
};

#ifdef CONFIG_OF
static struct of_device_id nvt_match_table[] = {
	{ .compatible = "novatek,NVT-ts",},
	{ },
};
#endif

static struct i2c_driver nvt_i2c_driver = {
	.probe		= nvt_ts_probe,
	.remove		= nvt_ts_remove,
	.id_table	= nvt_ts_id,
	.driver = {
		.name	= NVT_I2C_NAME,
		.owner	= THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = nvt_match_table,
#endif
	},
};


#define DOES_NOT_SUPPORT_SHUTDOWN_CHARGING

#ifdef DOES_NOT_SUPPORT_SHUTDOWN_CHARGING
extern char *saved_command_line;
#endif

static int32_t __init nvt_driver_init(void)
{
	int32_t ret = 0;

	g_nvt.valid = false;

#ifdef DOES_NOT_SUPPORT_SHUTDOWN_CHARGING
	if (strstr(saved_command_line, "androidboot.mode=charger") != NULL) {
		LOGV("androidboot.mode=charger, TP doesn't support!\n");
		goto err;
	}
#endif

	if (IS_ERR_OR_NULL(g_lcd_id)) {
		goto err;
	} else {
		if (strstr(g_lcd_id, "tianma nt36672a") != NULL) {
			NVT_LOG("TP info: [Vendor]tianma [IC]nt36672a\n");
		} else if (strstr(g_lcd_id, "shenchao nt36672a") != NULL) {
			NVT_LOG("TP info: [Vendor]shenchao [IC] nt36672a\n");
		} else {
			NVT_ERR("Touch IC is not nt36672a\n");
			goto err;
		}
	}

	ret = i2c_add_driver(&nvt_i2c_driver);
	if (ret)
		goto err;

	goto exit;

err:
	ret = -ENODEV;
exit:
	return ret;
}

static void __exit nvt_driver_exit(void)
{
	i2c_del_driver(&nvt_i2c_driver);

#if BOOT_UPDATE_FIRMWARE
	if (nvt_fwu_wq)
		destroy_workqueue(nvt_fwu_wq);
#endif
}

module_init(nvt_driver_init);
module_exit(nvt_driver_exit);
MODULE_DESCRIPTION("Novatek Touchscreen Driver");
MODULE_LICENSE("GPL");
