/*
 * Copyright Elliptic Labs
 * Copyright (C) 2019 XiaoMi, Inc.
 *
 */

#include "elliptic_device.h"
#include <sound/apr_elliptic.h>
#include <sound/q6afe-v2.h>

#define IO_PING_PONG_BUFFER_SIZE 512
#define AFE_MSM_RX_PSEUDOPORT_ID 0x8001
#define AFE_MSM_TX_PSEUDOPORT_ID 0x8002

struct elliptic_msm_io_device {};

int elliptic_data_io_initialize(void)
{
	return 0;
}

int elliptic_data_io_cleanup(void)
{
	return 0;
}
int elliptic_io_open_port(int portid)
{
	if (portid == ULTRASOUND_RX_PORT_ID)
		return afe_start_pseudo_port(AFE_MSM_RX_PSEUDOPORT_ID);
	else
		return afe_start_pseudo_port(AFE_MSM_TX_PSEUDOPORT_ID);
}

int elliptic_io_close_port(int portid)
{
	if (portid == ULTRASOUND_RX_PORT_ID)
		return afe_stop_pseudo_port(AFE_MSM_RX_PSEUDOPORT_ID);
	else
		return afe_stop_pseudo_port(AFE_MSM_TX_PSEUDOPORT_ID);
}

int32_t elliptic_data_io_write(uint32_t message_id, const char *data,
	size_t data_size)
{
	int32_t result = 0;

	result = ultrasound_apr_set_parameter(ELLIPTIC_PORT_ID,
		message_id, (u8 *)data,
		(int32_t)data_size);

	return result;
}

int32_t elliptic_data_io_transact(uint32_t message_id, const char *data,
	size_t data_size, char *output_data, size_t output_data_size)
{
	pr_err("%s : unimplemented\n", __func__);
	return -EINVAL;
}
