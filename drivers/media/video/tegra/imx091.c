/*
 * imx091.c - imx091 sensor driver
 *
 *  * Copyright (c) 2012 NVIDIA Corporation.  All rights reserved.
 *
 * Contributors:
 *	  Krupal Divvela <kdivvela@nvidia.com>
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2. This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 */

#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <media/imx091.h>
#include <linux/gpio.h>
#include <linux/module.h>

struct imx091_reg {
	u16 addr;
	u16 val;
};

struct imx091_info {
	int                         mode;
	struct imx091_sensordata    sensor_data;
	struct i2c_client           *i2c_client;
	struct imx091_platform_data *pdata;
	atomic_t                    in_use;
};

#define IMX091_TABLE_WAIT_MS 0
#define IMX091_TABLE_END 1
#define IMX091_MAX_RETRIES 3

#define IMX091_WAIT_MS 3

static struct imx091_reg mode_4208x3120[] = {
	/* Stand by */
	{0x0100, 0x00},
	{IMX091_TABLE_WAIT_MS, IMX091_WAIT_MS},

	/* global settings */
	{0x3087, 0x53},
	{0x309D, 0x94},
	{0x30A1, 0x08},
	{0x30C7, 0x00},
	{0x3115, 0x0E},
	{0x3118, 0x42},
	{0x311D, 0x34},
	{0x3121, 0x0D},
	{0x3212, 0xF2},
	{0x3213, 0x0F},
	{0x3215, 0x0F},
	{0x3217, 0x0B},
	{0x3219, 0x0B},
	{0x321B, 0x0D},
	{0x321D, 0x0D},

	/* black level setting */
	{0x3032, 0x40},

	/* PLL */
	{0x0305, 0x02},
	{0x0307, 0x2F},
	{0x30A4, 0x02},
	{0x303C, 0x4B},

	/* Mode Settings */
	{0x0112, 0x0A},
	{0x0113, 0x0A},
	{0x0340, 0x0C},
	{0x0341, 0x4A},
	{0x0342, 0x12},
	{0x0343, 0x0C},
	{0x0344, 0x00},
	{0x0345, 0x08},
	{0x0346, 0x00},
	{0x0347, 0x30},
	{0x0348, 0x10},
	{0x0349, 0x77},
	{0x034A, 0x0C},
	{0x034B, 0x5F},
	{0x034C, 0x10},
	{0x034D, 0x70},
	{0x034E, 0x0C},
	{0x034F, 0x30},
	{0x0381, 0x01},
	{0x0383, 0x01},
	{0x0385, 0x01},
	{0x0387, 0x01},
	{0x3033, 0x00},
	{0x303D, 0x10},
	{0x303E, 0xD0},
	{0x3040, 0x08},
	{0x3041, 0x97},
	{0x3048, 0x00},
	{0x304C, 0x7F},
	{0x304D, 0x04},
	{0x3064, 0x12},
	{0x309B, 0x20},
	{0x309E, 0x00},
	{0x30D5, 0x00},
	{0x30D6, 0x85},
	{0x30D7, 0x2A},
	{0x30D8, 0x64},
	{0x30D9, 0x89},
	{0x30DE, 0x00},
	{0x3102, 0x10},
	{0x3103, 0x44},
	{0x3104, 0x40},
	{0x3105, 0x00},
	{0x3106, 0x0D},
	{0x3107, 0x01},
	{0x310A, 0x0A},
	{0x315C, 0x99},
	{0x315D, 0x98},
	{0x316E, 0x9A},
	{0x316F, 0x99},
	{0x3301, 0x03},
	{0x3304, 0x05},
	{0x3305, 0x04},
	{0x3306, 0x12},
	{0x3307, 0x03},
	{0x3308, 0x0D},
	{0x3309, 0x05},
	{0x330A, 0x09},
	{0x330B, 0x04},
	{0x330C, 0x08},
	{0x330D, 0x05},
	{0x330E, 0x03},
	{0x3318, 0x64},
	{0x3322, 0x02},
	{0x3342, 0x0F},
	{0x3348, 0xE0},

	{0x0101, 0x03},

	/* stream on */
	{0x0100, 0x01},

	{IMX091_TABLE_WAIT_MS, IMX091_WAIT_MS},
	{IMX091_TABLE_END, 0x00}
};

static struct imx091_reg mode_2104x1560[] = {
	/* Stand by */
	{0x0100, 0x00},
	{IMX091_TABLE_WAIT_MS, IMX091_WAIT_MS},

	/* global settings */
	{0x3087, 0x53},
	{0x309D, 0x94},
	{0x30A1, 0x08},
	{0x30C7, 0x00},
	{0x3115, 0x0E},
	{0x3118, 0x42},
	{0x311D, 0x34},
	{0x3121, 0x0D},
	{0x3212, 0xF2},
	{0x3213, 0x0F},
	{0x3215, 0x0F},
	{0x3217, 0x0B},
	{0x3219, 0x0B},
	{0x321B, 0x0D},
	{0x321D, 0x0D},

	/* black level setting */
	{0x3032, 0x40},

	/* PLL */
	{0x0305, 0x02},
	{0x0307, 0x2F},
	{0x30A4, 0x02},
	{0x303C, 0x4B},

	/* Mode Settings */
	{0x0112, 0x0A},
	{0x0113, 0x0A},
	{0x0340, 0x06},
	{0x0341, 0x58},
	{0x0342, 0x12},
	{0x0343, 0x0C},
	{0x0344, 0x00},
	{0x0345, 0x08},
	{0x0346, 0x00},
	{0x0347, 0x30},
	{0x0348, 0x10},
	{0x0349, 0x77},
	{0x034A, 0x0C},
	{0x034B, 0x5F},
	{0x034C, 0x08},
	{0x034D, 0x38},
	{0x034E, 0x06},
	{0x034F, 0x18},
	{0x0381, 0x01},
	{0x0383, 0x03},
	{0x0385, 0x01},
	{0x0387, 0x03},
	{0x3033, 0x00},
	{0x303D, 0x10},
	{0x303E, 0xD0},
	{0x3040, 0x08},
	{0x3041, 0x97},
	{0x3048, 0x01},
	{0x304C, 0x7F},
	{0x304D, 0x04},
	{0x3064, 0x12},
	{0x309B, 0x28},
	{0x309E, 0x00},
	{0x30D5, 0x09},
	{0x30D6, 0x01},
	{0x30D7, 0x01},
	{0x30D8, 0x64},
	{0x30D9, 0x89},
	{0x30DE, 0x02},
	{0x3102, 0x10},
	{0x3103, 0x44},
	{0x3104, 0x40},
	{0x3105, 0x00},
	{0x3106, 0x0D},
	{0x3107, 0x01},
	{0x310A, 0x0A},
	{0x315C, 0x99},
	{0x315D, 0x98},
	{0x316E, 0x9A},
	{0x316F, 0x99},
	{0x3301, 0x03},
	{0x3304, 0x05},
	{0x3305, 0x04},
	{0x3306, 0x12},
	{0x3307, 0x03},
	{0x3308, 0x0D},
	{0x3309, 0x05},
	{0x330A, 0x09},
	{0x330B, 0x04},
	{0x330C, 0x08},
	{0x330D, 0x05},
	{0x330E, 0x03},
	{0x3318, 0x73},
	{0x3322, 0x02},
	{0x3342, 0x0F},
	{0x3348, 0xE0},

	{0x0101, 0x03},
	/* stream on */
	{0x0100, 0x01},

	{IMX091_TABLE_WAIT_MS, IMX091_WAIT_MS},
	{IMX091_TABLE_END, 0x00}
};

static struct imx091_reg mode_524x390[] = {
	/* Stand by */
	{0x0100, 0x00},
	{IMX091_TABLE_WAIT_MS, IMX091_WAIT_MS},

	/* global settings */
	{0x3087, 0x53},
	{0x309D, 0x94},
	{0x30A1, 0x08},
	{0x30C7, 0x00},
	{0x3115, 0x0E},
	{0x3118, 0x42},
	{0x311D, 0x34},
	{0x3121, 0x0D},
	{0x3212, 0xF2},
	{0x3213, 0x0F},
	{0x3215, 0x0F},
	{0x3217, 0x0B},
	{0x3219, 0x0B},
	{0x321B, 0x0D},
	{0x321D, 0x0D},

	/* black level setting */
	{0x3032, 0x40},

	/* PLL */
	{0x0305, 0x02},
	{0x0307, 0x2F},
	{0x30A4, 0x02},
	{0x303C, 0x4B},

	/* Mode Settings */
	{0x0112, 0x0A},
	{0x0113, 0x0A},
	{0x0340, 0x01},
	{0x0341, 0x96},
	{0x0342, 0x12},
	{0x0343, 0x0C},
	{0x0344, 0x00},
	{0x0345, 0x10},
	{0x0346, 0x00},
	{0x0347, 0x30},
	{0x0348, 0x10},
	{0x0349, 0x6F},
	{0x034A, 0x0C},
	{0x034B, 0x5F},
	{0x034C, 0x02},
	{0x034D, 0x0C},
	{0x034E, 0x01},
	{0x034F, 0x86},
	{0x0381, 0x09},
	{0x0383, 0x07},
	{0x0385, 0x09},
	{0x0387, 0x07},
	{0x3033, 0x00},
	{0x303D, 0x10},
	{0x303E, 0xD0},
	{0x3040, 0x08},
	{0x3041, 0x97},
	{0x3048, 0x01},
	{0x304C, 0x7F},
	{0x304D, 0x04},
	{0x3064, 0x12},
	{0x309B, 0x28},
	{0x309E, 0x00},
	{0x30D5, 0x09},
	{0x30D6, 0x00},
	{0x30D7, 0x00},
	{0x30D8, 0x00},
	{0x30D9, 0x00},
	{0x30DE, 0x08},
	{0x3102, 0x10},
	{0x3103, 0x44},
	{0x3104, 0x40},
	{0x3105, 0x00},
	{0x3106, 0x0D},
	{0x3107, 0x01},
	{0x310A, 0x0A},
	{0x315C, 0x99},
	{0x315D, 0x98},
	{0x316E, 0x9A},
	{0x316F, 0x99},
	{0x3301, 0x03},
	{0x3304, 0x03},
	{0x3305, 0x02},
	{0x3306, 0x09},
	{0x3307, 0x06},
	{0x3308, 0x1E},
	{0x3309, 0x05},
	{0x330A, 0x05},
	{0x330B, 0x04},
	{0x330C, 0x07},
	{0x330D, 0x06},
	{0x330E, 0x01},
	{0x3318, 0x44},
	{0x3322, 0x0E},
	{0x3342, 0x00},
	{0x3348, 0xE0},

	{0x0101, 0x03},
	/* stream on */
	{0x0100, 0x01},

	{IMX091_TABLE_WAIT_MS, IMX091_WAIT_MS},
	{IMX091_TABLE_END, 0x00}
};

static struct imx091_reg mode_348x260[] = {
	/* Stand by */
	{0x0100, 0x00},
	{IMX091_TABLE_WAIT_MS, IMX091_WAIT_MS},

	/* global settings */
	{0x3087, 0x53},
	{0x309D, 0x94},
	{0x30A1, 0x08},
	{0x30C7, 0x00},
	{0x3115, 0x0E},
	{0x3118, 0x42},
	{0x311D, 0x34},
	{0x3121, 0x0D},
	{0x3212, 0xF2},
	{0x3213, 0x0F},
	{0x3215, 0x0F},
	{0x3217, 0x0B},
	{0x3219, 0x0B},
	{0x321B, 0x0D},
	{0x321D, 0x0D},

	/* black level setting */
	{0x3032, 0x40},

	/* PLL */
	{0x0305, 0x02},
	{0x0307, 0x24},
	{0x30A4, 0x02},
	{0x303C, 0x4B},

	/* Mode Settings */
	{0x0112, 0x0A},
	{0x0113, 0x0A},
	{0x0340, 0x01},
	{0x0341, 0x36},
	{0x0342, 0x09},
	{0x0343, 0x06},
	{0x0344, 0x00},
	{0x0345, 0x18},
	{0x0346, 0x00},
	{0x0347, 0x30},
	{0x0348, 0x10},
	{0x0349, 0x67},
	{0x034A, 0x0C},
	{0x034B, 0x5F},
	{0x034C, 0x01},
	{0x034D, 0x5C},
	{0x034E, 0x01},
	{0x034F, 0x04},
	{0x0381, 0x05},
	{0x0383, 0x07},
	{0x0385, 0x0B},
	{0x0387, 0x0D},
	{0x3033, 0x84},
	{0x303D, 0x10},
	{0x303E, 0xD0},
	{0x3040, 0x08},
	{0x3041, 0x97},
	{0x3048, 0x01},
	{0x304C, 0x3F},
	{0x304D, 0x02},
	{0x3064, 0x12},
	{0x309B, 0x48},
	{0x309E, 0x04},
	{0x30D5, 0x0D},
	{0x30D6, 0x00},
	{0x30D7, 0x00},
	{0x30D8, 0x00},
	{0x30D9, 0x00},
	{0x30DE, 0x06},
	{0x3102, 0x09},
	{0x3103, 0x23},
	{0x3104, 0x24},
	{0x3105, 0x00},
	{0x3106, 0x8B},
	{0x3107, 0x00},
	{0x310A, 0x0A},
	{0x315C, 0x4A},
	{0x315D, 0x49},
	{0x316E, 0x4B},
	{0x316F, 0x4A},
	{0x3301, 0x03},
	{0x3304, 0x02},
	{0x3305, 0x00},
	{0x3306, 0x06},
	{0x3307, 0x04},
	{0x3308, 0x10},
	{0x3309, 0x02},
	{0x330A, 0x03},
	{0x330B, 0x01},
	{0x330C, 0x05},
	{0x330D, 0x03},
	{0x330E, 0x01},
	{0x3318, 0x44},
	{0x3322, 0x05},
	{0x3342, 0x00},
	{0x3348, 0xE0},

	{0x0101, 0x03},
	/* stream on */
	{0x0100, 0x01},

	{IMX091_TABLE_WAIT_MS, IMX091_WAIT_MS},
	{IMX091_TABLE_END, 0x00}
};

static struct imx091_reg mode_1948x1096[] = {
	/* Stand by */
	{0x0100, 0x00},
	{IMX091_TABLE_WAIT_MS, IMX091_WAIT_MS},

	/* global settings */
	{0x3087, 0x53},
	{0x309D, 0x94},
	{0x30A1, 0x08},
	{0x30C7, 0x00},
	{0x3115, 0x0E},
	{0x3118, 0x42},
	{0x311D, 0x34},
	{0x3121, 0x0D},
	{0x3212, 0xF2},
	{0x3213, 0x0F},
	{0x3215, 0x0F},
	{0x3217, 0x0B},
	{0x3219, 0x0B},
	{0x321B, 0x0D},
	{0x321D, 0x0D},

	/* black level setting */
	{0x3032, 0x40},

	/* PLL */
	{0x0305, 0x02},
	{0x0307, 0x24},
	{0x30A4, 0x02},
	{0x303C, 0x4B},

	/* Mode Settings */
	{0x0112, 0x0A},
	{0x0113, 0x0A},
	{0x0340, 0x04},
	{0x0341, 0x5A},
	{0x0342, 0x09},
	{0x0343, 0x06},
	{0x0344, 0x00},
	{0x0345, 0xA4},
	{0x0346, 0x02},
	{0x0347, 0x00},
	{0x0348, 0x0F},
	{0x0349, 0xDB},
	{0x034A, 0x0A},
	{0x034B, 0x8F},
	{0x034C, 0x07},
	{0x034D, 0x9C},
	{0x034E, 0x04},
	{0x034F, 0x48},
	{0x0381, 0x01},
	{0x0383, 0x01},
	{0x0385, 0x01},
	{0x0387, 0x03},
	{0x3033, 0x84},
	{0x303D, 0x10},
	{0x303E, 0xD0},
	{0x3040, 0x08},
	{0x3041, 0x97},
	{0x3048, 0x01},
	{0x304C, 0x3F},
	{0x304D, 0x02},
	{0x3064, 0x12},
	{0x309B, 0x48},
	{0x309E, 0x04},
	{0x30D5, 0x04},
	{0x30D6, 0x85},
	{0x30D7, 0x2A},
	{0x30D8, 0x64},
	{0x30D9, 0x89},
	{0x30DE, 0x00},
	{0x3102, 0x09},
	{0x3103, 0x23},
	{0x3104, 0x24},
	{0x3105, 0x00},
	{0x3106, 0x8B},
	{0x3107, 0x00},
	{0x310A, 0x0A},
	{0x315C, 0x4A},
	{0x315D, 0x49},
	{0x316E, 0x4B},
	{0x316F, 0x4A},
	{0x3301, 0x03},
	{0x3304, 0x05},
	{0x3305, 0x04},
	{0x3306, 0x12},
	{0x3307, 0x03},
	{0x3308, 0x0D},
	{0x3309, 0x05},
	{0x330A, 0x09},
	{0x330B, 0x04},
	{0x330C, 0x08},
	{0x330D, 0x05},
	{0x330E, 0x03},
	{0x3318, 0x66},
	{0x3322, 0x02},
	{0x3342, 0x0F},
	{0x3348, 0xE0},

	{0x0101, 0x03},
	/* stream on */
	{0x0100, 0x01},

	{IMX091_TABLE_WAIT_MS, IMX091_WAIT_MS},
	{IMX091_TABLE_END, 0x00}
};

static struct imx091_reg mode_1308x736[] = {
	/* Stand by */
	{0x0100, 0x00},
	{IMX091_TABLE_WAIT_MS, IMX091_WAIT_MS},

	/* global settings */
	{0x3087, 0x53},
	{0x309D, 0x94},
	{0x30A1, 0x08},
	{0x30C7, 0x00},
	{0x3115, 0x0E},
	{0x3118, 0x42},
	{0x311D, 0x34},
	{0x3121, 0x0D},
	{0x3212, 0xF2},
	{0x3213, 0x0F},
	{0x3215, 0x0F},
	{0x3217, 0x0B},
	{0x3219, 0x0B},
	{0x321B, 0x0D},
	{0x321D, 0x0D},

	/* black level setting */
	{0x3032, 0x40},

	/* PLL */
	{0x0305, 0x02},
	{0x0307, 0x2F},
	{0x30A4, 0x02},
	{0x303C, 0x4B},

	/* Mode Settings */
	{0x0112, 0x0A},
	{0x0113, 0x0A},
	{0x0340, 0x02},
	{0x0341, 0xF0},
	{0x0342, 0x12},
	{0x0343, 0x0C},
	{0x0344, 0x00},
	{0x0345, 0x96},
	{0x0346, 0x01},
	{0x0347, 0xF8},
	{0x0348, 0x0F},
	{0x0349, 0xE9},
	{0x034A, 0x0A},
	{0x034B, 0x97},
	{0x034C, 0x05},
	{0x034D, 0x1C},
	{0x034E, 0x02},
	{0x034F, 0xE0},
	{0x0381, 0x03},
	{0x0383, 0x03},
	{0x0385, 0x03},
	{0x0387, 0x03},
	{0x3033, 0x00},
	{0x303D, 0x10},
	{0x303E, 0xD0},
	{0x3040, 0x08},
	{0x3041, 0x97},
	{0x3048, 0x22},
	{0x304C, 0x7F},
	{0x304D, 0x04},
	{0x3064, 0x12},
	{0x309B, 0x60},
	{0x309E, 0x04},
	{0x30D5, 0x09},
	{0x30D6, 0x00},
	{0x30D7, 0x00},
	{0x30D8, 0x00},
	{0x30D9, 0x89},
	{0x30DE, 0x03},
	{0x3102, 0x09},
	{0x3103, 0x23},
	{0x3104, 0x24},
	{0x3105, 0x00},
	{0x3106, 0x8B},
	{0x3107, 0x00},
	{0x310A, 0x0A},
	{0x315C, 0x4A},
	{0x315D, 0x49},
	{0x316E, 0x4B},
	{0x316F, 0x4A},
	{0x3301, 0x03},
	{0x3304, 0x05},
	{0x3305, 0x04},
	{0x3306, 0x12},
	{0x3307, 0x03},
	{0x3308, 0x0D},
	{0x3309, 0x05},
	{0x330A, 0x09},
	{0x330B, 0x04},
	{0x330C, 0x08},
	{0x330D, 0x05},
	{0x330E, 0x03},
	{0x3318, 0x6A},
	{0x3322, 0x02},
	{0x3342, 0x0F},
	{0x3348, 0xE0},

	{0x0101, 0x03},
	/* stream on */
	{0x0100, 0x01},

	{IMX091_TABLE_WAIT_MS, IMX091_WAIT_MS},
	{IMX091_TABLE_END, 0x00}
};

enum {
	IMX091_MODE_4208X3120,
	IMX091_MODE_2104X1560,
	IMX091_MODE_524X390,
	IMX091_MODE_348X260,
	IMX091_MODE_1948X1096,
	IMX091_MODE_1308X736,
};

static struct imx091_reg *mode_table[] = {
	[IMX091_MODE_4208X3120] = mode_4208x3120,
	[IMX091_MODE_2104X1560] = mode_2104x1560,
	[IMX091_MODE_524X390]   = mode_524x390,
	[IMX091_MODE_348X260]   = mode_348x260,
	[IMX091_MODE_1948X1096] = mode_1948x1096,
	[IMX091_MODE_1308X736]  = mode_1308x736,
};

static inline void
msleep_range(unsigned int delay_base)
{
	usleep_range(delay_base*1000, delay_base*1000+500);
}

static inline void
imx091_get_frame_length_regs(struct imx091_reg *regs, u32 frame_length)
{
	regs->addr = 0x0340;
	regs->val = (frame_length >> 8) & 0xff;
	(regs + 1)->addr = 0x0341;
	(regs + 1)->val = (frame_length) & 0xff;
}

static inline void
imx091_get_coarse_time_regs(struct imx091_reg *regs, u32 coarse_time)
{
	regs->addr = 0x202;
	regs->val = (coarse_time >> 8) & 0xff;
	(regs + 1)->addr = 0x203;
	(regs + 1)->val = (coarse_time) & 0xff;
}

static inline void
imx091_get_gain_reg(struct imx091_reg *regs, u16 gain)
{
	regs->addr = 0x205;
	regs->val = gain;
}

static int
imx091_read_reg(struct i2c_client *client, u16 addr, u8 *val)
{
	int err;
	struct i2c_msg msg[2];
	unsigned char data[3];

	if (!client->adapter)
		return -ENODEV;

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = 2;
	msg[0].buf = data;

	/* high byte goes out first */
	data[0] = (u8) (addr >> 8);
	data[1] = (u8) (addr & 0xff);

	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 1;
	msg[1].buf = data + 2;

	err = i2c_transfer(client->adapter, msg, 2);

	if (err != 2)
		return -EINVAL;

	*val = data[2];
	return 0;
}

static int
imx091_write_reg(struct i2c_client *client, u16 addr, u8 val)
{
	int err;
	struct i2c_msg msg;
	unsigned char data[3];
	int retry = 0;

	if (!client->adapter)
		return -ENODEV;

	data[0] = (u8) (addr >> 8);
	data[1] = (u8) (addr & 0xff);
	data[2] = (u8) (val & 0xff);

	msg.addr = client->addr;
	msg.flags = 0;
	msg.len = 3;
	msg.buf = data;

	do {
		err = i2c_transfer(client->adapter, &msg, 1);
		if (err == 1)
			return 0;
		retry++;
		pr_err("[IMX091]:%s:i2c transfer failed, retrying %x %x\n",
		       __func__, addr, val);
		msleep_range(3);
	} while (retry <= IMX091_MAX_RETRIES);

	return err;
}

static int
imx091_write_table(struct i2c_client *client,
				 const struct imx091_reg table[],
				 const struct imx091_reg override_list[],
				 int num_override_regs)
{
	int err;
	const struct imx091_reg *next;
	int i;
	u16 val;

	for (next = table; next->addr != IMX091_TABLE_END; next++) {
		if (next->addr == IMX091_TABLE_WAIT_MS) {
			msleep_range(next->val);
			continue;
		}

		val = next->val;

		/* When an override list is passed in, replace the reg */
		/* value to write if the reg is in the list            */
		if (override_list) {
			for (i = 0; i < num_override_regs; i++) {
				if (next->addr == override_list[i].addr) {
					val = override_list[i].val;
					break;
				}
			}
		}

		err = imx091_write_reg(client, next->addr, val);
		if (err) {
			pr_err("[IMX091]:%s:imx091_write_table:%d", __func__, err);
			return err;
		}
	}
	return 0;
}

static int
imx091_set_mode(struct imx091_info *info, struct imx091_mode *mode)
{
	int sensor_mode;
	int err;
	struct imx091_reg reg_list[5];

	pr_info("[IMX091]:%s: xres %u yres %u framelength %u coarsetime %u gain %u\n",
			 __func__, mode->xres, mode->yres, mode->frame_length,
			 mode->coarse_time, mode->gain);

	if (mode->xres == 4208 && mode->yres == 3120)
		sensor_mode = IMX091_MODE_4208X3120;
	else if (mode->xres == 2104 && mode->yres == 1560)
		sensor_mode = IMX091_MODE_2104X1560;
	else if (mode->xres == 524 && mode->yres == 374)
		sensor_mode = IMX091_MODE_524X390;
	else if (mode->xres == 348 && mode->yres == 260)
		sensor_mode = IMX091_MODE_348X260;
	else if (mode->xres == 1948 && mode->yres == 1096)
		sensor_mode = IMX091_MODE_1948X1096;
	else if (mode->xres == 1308 && mode->yres == 736)
		sensor_mode = IMX091_MODE_1308X736;
	else {
		pr_err("[IMX091]:%s: invalid resolution supplied to set mode %d %d\n",
			 __func__, mode->xres, mode->yres);
		return -EINVAL;
	}

	/* get a list of override regs for the asking frame length, */
	/* coarse integration time, and gain.                       */
	imx091_get_frame_length_regs(reg_list, mode->frame_length);
	imx091_get_coarse_time_regs(reg_list + 2, mode->coarse_time);
	imx091_get_gain_reg(reg_list + 4, mode->gain);

	err = imx091_write_table(info->i2c_client, mode_table[sensor_mode],
							 reg_list, 5);
	if (err)
		return err;

	info->mode = sensor_mode;
	pr_info("[IMX091]: stream on.\n");
	return 0;
}

static int
imx091_get_status(struct imx091_info *info, u8 *dev_status)
{
	*dev_status = 0;
	return 0;
}

static int
imx091_set_frame_length(struct imx091_info *info, u32 frame_length,
						 bool group_hold)
{
	struct imx091_reg reg_list[2];
	int i = 0;
	int ret;

	imx091_get_frame_length_regs(reg_list, frame_length);

	if (group_hold) {
		ret = imx091_write_reg(info->i2c_client, 0x0104, 0x01);
		if (ret)
			return ret;
	}

	for (i = 0; i < 2; i++) {
		ret = imx091_write_reg(info->i2c_client, reg_list[i].addr,
			 reg_list[i].val);
		if (ret)
			return ret;
	}

	if (group_hold) {
		ret = imx091_write_reg(info->i2c_client, 0x0104, 0x0);
		if (ret)
			return ret;
	}

	return 0;
}

static int
imx091_set_coarse_time(struct imx091_info *info, u32 coarse_time,
						 bool group_hold)
{
	int ret;

	struct imx091_reg reg_list[2];
	int i = 0;

	imx091_get_coarse_time_regs(reg_list, coarse_time);

	if (group_hold) {
		ret = imx091_write_reg(info->i2c_client, 0x104, 0x01);
		if (ret)
			return ret;
	}

	for (i = 0; i < 2; i++) {
		ret = imx091_write_reg(info->i2c_client, reg_list[i].addr,
			 reg_list[i].val);
		if (ret)
			return ret;
	}

	if (group_hold) {
		ret = imx091_write_reg(info->i2c_client, 0x104, 0x0);
		if (ret)
			return ret;
	}
	return 0;
}

static int
imx091_set_gain(struct imx091_info *info, u16 gain, bool group_hold)
{
	int ret;
	struct imx091_reg reg_list;

	imx091_get_gain_reg(&reg_list, gain);

	if (group_hold) {
		ret = imx091_write_reg(info->i2c_client, 0x104, 0x1);
		if (ret)
			return ret;
	}

	ret = imx091_write_reg(info->i2c_client, reg_list.addr, reg_list.val);
	if (ret)
		return ret;

	if (group_hold) {
		ret = imx091_write_reg(info->i2c_client, 0x104, 0x0);
		if (ret)
			return ret;
	}
	return 0;
}

static int
imx091_set_group_hold(struct imx091_info *info, struct imx091_ae *ae)
{
	int ret;
	int count = 0;
	bool groupHoldEnabled = false;

	if (ae->gain_enable)
		count++;
	if (ae->coarse_time_enable)
		count++;
	if (ae->frame_length_enable)
		count++;
	if (count >= 2)
		groupHoldEnabled = true;

	if (groupHoldEnabled) {
		ret = imx091_write_reg(info->i2c_client, 0x104, 0x1);
		if (ret)
			return ret;
	}

	if (ae->gain_enable)
		imx091_set_gain(info, ae->gain, false);
	if (ae->coarse_time_enable)
		imx091_set_coarse_time(info, ae->coarse_time, false);
	if (ae->frame_length_enable)
		imx091_set_frame_length(info, ae->frame_length, false);

	if (groupHoldEnabled) {
		ret = imx091_write_reg(info->i2c_client, 0x104, 0x0);
		if (ret)
			return ret;
	}

	return 0;
}

static int imx091_get_sensor_id(struct imx091_info *info)
{
	int ret = 0;
	int i;
	u8 bak = 0;

	pr_info("%s\n", __func__);
	if (info->sensor_data.fuse_id_size)
		return 0;

	/* Note 1: If the sensor does not have power at this point
	Need to supply the power, e.g. by calling power on function */

	ret |= imx091_write_reg(info->i2c_client, 0x34C9, 0x10);
	for (i = 0; i < 8 ; i++) {
		ret |= imx091_read_reg(info->i2c_client, 0x3580 + i, &bak);
		info->sensor_data.fuse_id[i] = bak;
	}

	if (!ret)
		info->sensor_data.fuse_id_size = i;

	/* Note 2: Need to clean up any action carried out in Note 1 */

	return ret;
}

static long
imx091_ioctl(struct file *file,
			 unsigned int cmd, unsigned long arg)
{
	int err;
	struct imx091_info *info = file->private_data;

	switch (cmd) {
	case IMX091_IOCTL_SET_MODE:
	{
		struct imx091_mode mode;
		if (copy_from_user(&mode,
			 (const void __user *)arg,
			 sizeof(struct imx091_mode))) {
			pr_err("[IMX091]:%s:Failed to get mode from user.\n", __func__);
			return -EFAULT;
		}
		return imx091_set_mode(info, &mode);
	}
	case IMX091_IOCTL_SET_FRAME_LENGTH:
		return imx091_set_frame_length(info, (u32)arg, true);
	case IMX091_IOCTL_SET_COARSE_TIME:
		return imx091_set_coarse_time(info, (u32)arg, true);
	case IMX091_IOCTL_SET_GAIN:
		return imx091_set_gain(info, (u16)arg, true);
	case IMX091_IOCTL_GET_STATUS:
	{
		u8 status;

		err = imx091_get_status(info, &status);
		if (err)
			return err;
		if (copy_to_user((void __user *)arg, &status, 1)) {
			pr_err("[IMX091]:%s:Failed to copy status to user.\n", __func__);
			return -EFAULT;
			}
		return 0;
	}
	case IMX091_IOCTL_GET_SENSORDATA:
	{
		err = imx091_get_sensor_id(info);

		if (err) {
			pr_err("[IMX091]:%s:Failed to get fuse id info.\n", __func__);
			return err;
		}
		if (copy_to_user((void __user *)arg,
						 &info->sensor_data,
						 sizeof(struct imx091_sensordata))) {
			pr_info("[IMX091]:%s:Failed to copy fuse id to user space\n",
					__func__);
			return -EFAULT;
		}
		return 0;
	}
	case IMX091_IOCTL_SET_GROUP_HOLD:
	{
	  struct imx091_ae ae;
	  if (copy_from_user(&ae, (const void __user *)arg,
			sizeof(struct imx091_ae))) {
		pr_info("[IMX091]:%s:fail group hold\n", __func__);
		return -EFAULT;
	  }
	  return imx091_set_group_hold(info, &ae);
	}
	default:
	  pr_err("[IMX091]:%s:unknown cmd.\n", __func__);
	  return -EINVAL;
	}
	return 0;
}

static struct imx091_info *info;

static int
imx091_open(struct inode *inode, struct file *file)
{
	/* check if the device is in use */
	if (atomic_xchg(&info->in_use, 1)) {
		pr_info("[IMX091]:%s:BUSY!\n", __func__);
		return -EBUSY;
	}

	file->private_data = info;

	if (info->pdata && info->pdata->power_on)
		info->pdata->power_on();
	else{
		pr_err("[IMX091]:%s:no valid power_on function.\n", __func__);
		return -EEXIST;
	}

	return 0;
}

static int
imx091_release(struct inode *inode, struct file *file)
{
	if (info->pdata && info->pdata->power_off)
		info->pdata->power_off();
	file->private_data = NULL;

	/* warn if device is already released */
	WARN_ON(!atomic_xchg(&info->in_use, 0));
	return 0;
}


static const struct file_operations imx091_fileops = {
	.owner = THIS_MODULE,
	.open = imx091_open,
	.unlocked_ioctl = imx091_ioctl,
	.release = imx091_release,
};

static struct miscdevice imx091_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "imx091",
	.fops = &imx091_fileops,
};

static int
imx091_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	int err;

	pr_info("[IMX091]: probing sensor.\n");

	info = kzalloc(sizeof(struct imx091_info), GFP_KERNEL);
	if (!info) {
		pr_err("[IMX091]:%s:Unable to allocate memory!\n", __func__);
		return -ENOMEM;
	}

	err = misc_register(&imx091_device);
	if (err) {
		pr_err("[IMX091]:%s:Unable to register misc device!\n", __func__);
		kfree(info);
		return err;
	}

	info->pdata = client->dev.platform_data;
	info->i2c_client = client;
	atomic_set(&info->in_use, 0);
	info->mode = -1;

	i2c_set_clientdata(client, info);
	return 0;
}

static int
imx091_remove(struct i2c_client *client)
{
	struct imx091_info *info;
	info = i2c_get_clientdata(client);
	misc_deregister(&imx091_device);
	kfree(info);
	return 0;
}

static const struct i2c_device_id imx091_id[] = {
	{ "imx091", 0 },
};

MODULE_DEVICE_TABLE(i2c, imx091_id);

static struct i2c_driver imx091_i2c_driver = {
	.driver = {
		.name = "imx091",
		.owner = THIS_MODULE,
	},
	.probe = imx091_probe,
	.remove = imx091_remove,
	.id_table = imx091_id,
};

static int __init
imx091_init(void)
{
	pr_info("[IMX091] sensor driver loading\n");
	return i2c_add_driver(&imx091_i2c_driver);
}

static void __exit
imx091_exit(void)
{
	i2c_del_driver(&imx091_i2c_driver);
}

module_init(imx091_init);
module_exit(imx091_exit);