/*
 *
 * FocalTech TouchScreen driver.
 *
 * Copyright (c) 2010-2017, FocalTech Systems, Ltd., all rights reserved.
 * Copyright (C) 2019 XiaoMi, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _LINUX_FOCLATECH_CONFIG_H_
#define _LINUX_FOCLATECH_CONFIG_H_

#define _FT8716             0x87160805
#define _FT8736             0x87360806
#define _FT8006M            0x80060807
#define _FT8201             0x82010807
#define _FT7250             0x72500807
#define _FT8606             0x86060808
#define _FT8607             0x86070809
#define _FTE716             0xE716080A
#define _FT8006U            0x8006D80B
#define _FT8613             0x8613080C
#define _FT8719             0x8719080D

#define _FT5416             0x54160402
#define _FT5426             0x54260402
#define _FT5435             0x54350402
#define _FT5436             0x54360402
#define _FT5526             0x55260402
#define _FT5526I            0x5526B402
#define _FT5446             0x54460402
#define _FT5346             0x53460402
#define _FT5446I            0x5446B402
#define _FT5346I            0x5346B402
#define _FT7661             0x76610402
#define _FT7511             0x75110402
#define _FT7421             0x74210402
#define _FT7681             0x76810402
#define _FT3C47U            0x3C47D402
#define _FT3417             0x34170402
#define _FT3517             0x35170402
#define _FT3327             0x33270402
#define _FT3427             0x34270402
#define _FT7311             0x73110402

#define _FT5626             0x56260401
#define _FT5726             0x57260401
#define _FT5826B            0x5826B401
#define _FT5826S            0x5826C401
#define _FT7811             0x78110401
#define _FT3D47             0x3D470401
#define _FT3617             0x36170401
#define _FT3717             0x37170401
#define _FT3817B            0x3817B401
#define _FT3517U            0x3517D401

#define _FT6236U            0x6236D003
#define _FT6336G            0x6336A003
#define _FT6336U            0x6336D003
#define _FT6436U            0x6436D003

#define _FT3267             0x32670004
#define _FT3367             0x33670004
#define _FT5422U            0x5422D482
#define _FT3327DQQ_001      0x3327D482

#define FTS_CHIP_TYPE   _FT8719
#define FTS_MT_PROTOCOL_B_EN                    1
#define FTS_REPORT_PRESSURE_EN                  0
#define FTS_GESTURE_EN                          1
#define FTS_GLOVE_EN                            1
#define FTS_COVER_EN                            0
#define FTS_CHARGER_EN                          0
#define FTS_SYSFS_NODE_EN                       1
#define FTS_APK_NODE_EN                         1
#define FTS_TP_LOCKDOWN_INFO                    1
#define FTS_PINCTRL_EN                          0
#define FTS_POWER_SOURCE_CUST_EN                0
#define FTS_AUTO_UPGRADE_EN                     0
#define FTS_AUTO_LIC_UPGRADE_EN                 0
#define FTS_GET_VENDOR_ID_NUM                   0

#define FTS_VENDOR_ID                          0x0000
#define FTS_VENDOR_ID2                         0x0000
#define FTS_VENDOR_ID3                         0x0000

#define FIRMWARE                      "include/firmware/LQ_E7T_FT8719_BOE_VID0xDA_PID0xC3_6P26_V0x09_L0xA5_I2C_20180928_app.i"

#define FTS_UPGRADE_FW_FILE           FIRMWARE
#define FTS_UPGRADE_FW2_FILE          FIRMWARE
#define FTS_UPGRADE_FW3_FILE          FIRMWARE

#endif /* _LINUX_FOCLATECH_CONFIG_H_ */
