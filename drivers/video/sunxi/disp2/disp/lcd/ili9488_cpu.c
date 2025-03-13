/*
 * drivers/video/fbdev/sunxi/disp2/disp/lcd/ili9488_cpu.c
 *
 * Copyright (c) 2007-2018 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
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
 *
 */

#include "ili9488_cpu.h"

#define CPU_TRI_MODE

#define DBG_INFO(format, args...) 	
#define DBG_ERR(format, args...) 
#define panel_reset(val) 		sunxi_lcd_gpio_set_value(0, 0, val)
#define lcd_cs(val)  			sunxi_lcd_gpio_set_value(0, 1, val)

static void lcd_panel_ili9488_init(u32 sel);
static void LCD_power_on(u32 sel);
static void LCD_power_off(u32 sel);
static void LCD_bl_open(u32 sel);
static void LCD_bl_close(u32 sel);

static void LCD_panel_init(u32 sel);
static void LCD_panel_exit(u32 sel);

static void LCD_cfg_panel_info(panel_extend_para *info)
{
#if 1
	u32 i = 0, j = 0;
	u32 items;
	u8 lcd_gamma_tbl[][2] = {
	    //{input value, corrected value}
	    {0, 0},     {15, 15},   {30, 30},   {45, 45},   {60, 60},
	    {75, 75},   {90, 90},   {105, 105}, {120, 120}, {135, 135},
	    {150, 150}, {165, 165}, {180, 180}, {195, 195}, {210, 210},
	    {225, 225}, {240, 240}, {255, 255},
	};

	u32 lcd_cmap_tbl[2][3][4] = {
	    {
		{LCD_CMAP_G0, LCD_CMAP_B1, LCD_CMAP_G2, LCD_CMAP_B3},
		{LCD_CMAP_B0, LCD_CMAP_R1, LCD_CMAP_B2, LCD_CMAP_R3},
		{LCD_CMAP_R0, LCD_CMAP_G1, LCD_CMAP_R2, LCD_CMAP_G3},
	    },
	    {
		{LCD_CMAP_B3, LCD_CMAP_G2, LCD_CMAP_B1, LCD_CMAP_G0},
		{LCD_CMAP_R3, LCD_CMAP_B2, LCD_CMAP_R1, LCD_CMAP_B0},
		{LCD_CMAP_G3, LCD_CMAP_R2, LCD_CMAP_G1, LCD_CMAP_R0},
	    },
	};

	items = sizeof(lcd_gamma_tbl) / 2;
	for (i = 0; i < items - 1; i++) {
		u32 num = lcd_gamma_tbl[i + 1][0] - lcd_gamma_tbl[i][0];

		for (j = 0; j < num; j++) {
			u32 value = 0;

			value =
			    lcd_gamma_tbl[i][1] +
			    ((lcd_gamma_tbl[i + 1][1] - lcd_gamma_tbl[i][1]) *
			     j) /
				num;
			info->lcd_gamma_tbl[lcd_gamma_tbl[i][0] + j] =
			    (value << 16) + (value << 8) + value;
		}
	}
	info->lcd_gamma_tbl[255] = (lcd_gamma_tbl[items - 1][1] << 16) +
				   (lcd_gamma_tbl[items - 1][1] << 8) +
				   lcd_gamma_tbl[items - 1][1];

	memcpy(info->lcd_cmap_tbl, lcd_cmap_tbl, sizeof(lcd_cmap_tbl));
#endif
}

static s32 LCD_open_flow(u32 sel)
{
	LCD_OPEN_FUNC(sel, LCD_power_on, 120);
	LCD_OPEN_FUNC(sel, LCD_panel_init, 100);
	LCD_OPEN_FUNC(sel, sunxi_lcd_tcon_enable, 50);
	LCD_OPEN_FUNC(sel, LCD_bl_open, 0);
	return 0;
}

static s32 LCD_close_flow(u32 sel)
{
	LCD_CLOSE_FUNC(sel, LCD_bl_close, 20);
	LCD_CLOSE_FUNC(sel, sunxi_lcd_tcon_disable, 10);
	LCD_CLOSE_FUNC(sel, LCD_panel_exit, 50);
	LCD_CLOSE_FUNC(sel, LCD_power_off, 0);

	return 0;
}

static void LCD_power_on(u32 sel)
{
	/*config lcd_power pin to open lcd power0 */
	sunxi_lcd_power_enable(sel, 0);
	sunxi_lcd_pin_cfg(sel, 1);

	/*lcd_cs, active low */
	lcd_cs(0);
	sunxi_lcd_delay_ms(10);
	panel_reset(1);
	sunxi_lcd_delay_ms(20);
	panel_reset(0);
	sunxi_lcd_delay_ms(20);
	panel_reset(1);
	sunxi_lcd_delay_ms(120);

}

static void LCD_power_off(u32 sel)
{
	/*lcd_cs, active low */
	lcd_cs(1);
	sunxi_lcd_delay_ms(10);
	/*lcd_rst, active hight */
	panel_reset(1);
	sunxi_lcd_delay_ms(10);

	sunxi_lcd_pin_cfg(sel, 0);
	/*config lcd_power pin to close lcd power0 */
	sunxi_lcd_power_disable(sel, 0);
}

static void LCD_bl_open(u32 sel)
{
	sunxi_lcd_pwm_enable(sel);
	/*config lcd_bl_en pin to open lcd backlight */
	sunxi_lcd_backlight_enable(sel);

}

static void LCD_bl_close(u32 sel)
{
	/*config lcd_bl_en pin to close lcd backlight */
	sunxi_lcd_backlight_disable(sel);
	sunxi_lcd_pwm_disable(sel);
}

/*static int bootup_flag = 0;*/
static void LCD_panel_init(u32 sel)
{
	lcd_panel_ili9488_init(sel);
	return;
}

static void LCD_panel_exit(u32 sel)
{
	sunxi_lcd_cpu_write_index(0, 0x28);
	sunxi_lcd_delay_ms(10);
	sunxi_lcd_cpu_write_index(0, 0x10);
	sunxi_lcd_delay_ms(120);
}
#if 0
static void lcd_dbi_wr_dcs(__u32 sel, __u8 cmd, __u8 *para, __u32 para_num)
{
	__u8 index = cmd;
	__u8 *data_p = para;
	__u16 i;
	sunxi_lcd_cpu_write_index(sel, index);
	for (i = 0; i < para_num; i++) {
		sunxi_lcd_cpu_write_data(sel, *(data_p++));
	}
}

static void lcd_cpu_panel_fr(__u32 sel, __u32 w, __u32 h, __u32 x, __u32 y)
{
	__u8 para[4];
	__u32 para_num;
	para[0] = (x >> 8) & 0xff;
	para[1] = (x >> 0) & 0xff;
	para[2] = ((x + w - 1) >> 8) & 0xff;
	para[3] = ((x + w - 1) >> 0) & 0xff;
	para_num = 4;
	lcd_dbi_wr_dcs(sel, DSI_DCS_SET_COLUMN_ADDRESS, para, para_num);

	para[0] = (y >> 8) & 0xff;
	para[1] = (y >> 0) & 0xff;
	para[2] = ((y + h - 1) >> 8) & 0xff;
	para[3] = ((y + h - 1) >> 0) & 0xff;
	para_num = 4;
	lcd_dbi_wr_dcs(sel, DSI_DCS_SET_PAGE_ADDRESS, para, para_num);
}
#endif

static void lcd_panel_ili9488_init(u32 sel)
{
	sunxi_lcd_cpu_write_index(0, 0xF7);
	sunxi_lcd_cpu_write_data(0, 0xA9);
	sunxi_lcd_cpu_write_data(0, 0x51);
	sunxi_lcd_cpu_write_data(0, 0x2C);
	sunxi_lcd_cpu_write_data(0, 0x82);

	sunxi_lcd_cpu_write_index(0, 0xec);
	sunxi_lcd_cpu_write_data(0, 0x00);
	sunxi_lcd_cpu_write_data(0, 0x02);
	sunxi_lcd_cpu_write_data(0, 0x03);
	sunxi_lcd_cpu_write_data(0, 0x7a);

	sunxi_lcd_cpu_write_index(0, 0xC0);
	sunxi_lcd_cpu_write_data(0, 0x13);
	sunxi_lcd_cpu_write_data(0, 0x13);

	sunxi_lcd_cpu_write_index(0, 0xC1);
	sunxi_lcd_cpu_write_data(0, 0x41);

	sunxi_lcd_cpu_write_index(0, 0xC5);
	sunxi_lcd_cpu_write_data(0, 0x00);
	sunxi_lcd_cpu_write_data(0, 0x28);	//0x0a
	sunxi_lcd_cpu_write_data(0, 0x80);

	sunxi_lcd_cpu_write_index(0, 0xB1);	// frame rate 70hz
	sunxi_lcd_cpu_write_data(0, 0xB0);
	sunxi_lcd_cpu_write_data(0, 0x11);

	sunxi_lcd_cpu_write_index(0, 0xB4);
	sunxi_lcd_cpu_write_data(0, 0x02);

	sunxi_lcd_cpu_write_index(0, 0xB6);	// rgb/mcu interface control
	sunxi_lcd_cpu_write_data(0, 0x02);
	sunxi_lcd_cpu_write_data(0, 0x22);	//0x42

	sunxi_lcd_cpu_write_index(0, 0xB7);
	sunxi_lcd_cpu_write_data(0, 0xC6);

	sunxi_lcd_cpu_write_index(0, 0xBE);
	sunxi_lcd_cpu_write_data(0, 0x00);
	sunxi_lcd_cpu_write_data(0, 0x04);

	sunxi_lcd_cpu_write_index(0, 0xE9);
	sunxi_lcd_cpu_write_data(0, 0x00);

	sunxi_lcd_cpu_write_index(0, 0xf4);
	sunxi_lcd_cpu_write_data(0, 0x00);
	sunxi_lcd_cpu_write_data(0, 0x00);
	sunxi_lcd_cpu_write_data(0, 0x0f);

	sunxi_lcd_cpu_write_index(0, 0xE0);
	sunxi_lcd_cpu_write_data(0, 0x00);
	sunxi_lcd_cpu_write_data(0, 0x04);
	sunxi_lcd_cpu_write_data(0, 0x0e);	//0x10
	sunxi_lcd_cpu_write_data(0, 0x08);	//0x09	
	sunxi_lcd_cpu_write_data(0, 0x17);	//0x17
	sunxi_lcd_cpu_write_data(0, 0x0a);
	sunxi_lcd_cpu_write_data(0, 0x40);	//0x41
	sunxi_lcd_cpu_write_data(0, 0x79);	//0x89
	sunxi_lcd_cpu_write_data(0, 0x4d);
	sunxi_lcd_cpu_write_data(0, 0x07);	//0x0a
	sunxi_lcd_cpu_write_data(0, 0x0e);	//0x0c
	sunxi_lcd_cpu_write_data(0, 0x0a);	//0x0e
	sunxi_lcd_cpu_write_data(0, 0x1a);	//0x18
	sunxi_lcd_cpu_write_data(0, 0x1d);	//0x1b
	sunxi_lcd_cpu_write_data(0, 0x0F);

	sunxi_lcd_cpu_write_index(0, 0xE1);
	sunxi_lcd_cpu_write_data(0, 0x00);
	sunxi_lcd_cpu_write_data(0, 0x1b);	//0x17
	sunxi_lcd_cpu_write_data(0, 0x1f);	//0x1a
	sunxi_lcd_cpu_write_data(0, 0x02);	//0x04
	sunxi_lcd_cpu_write_data(0, 0x10);	//0x0e
	sunxi_lcd_cpu_write_data(0, 0x05);	
	sunxi_lcd_cpu_write_data(0, 0x32);	//0x2f
	sunxi_lcd_cpu_write_data(0, 0x34);	//0x45
	sunxi_lcd_cpu_write_data(0, 0x43);	//0x43
	sunxi_lcd_cpu_write_data(0, 0x02);	//0x02
	sunxi_lcd_cpu_write_data(0, 0x0a);	//0x0a
	sunxi_lcd_cpu_write_data(0, 0x09);	//0x09
	sunxi_lcd_cpu_write_data(0, 0x33);	//0x32
	sunxi_lcd_cpu_write_data(0, 0x37);
	sunxi_lcd_cpu_write_data(0, 0x0F);

	sunxi_lcd_cpu_write_index(0, 0xf4);
	sunxi_lcd_cpu_write_data(0, 0x00);
	sunxi_lcd_cpu_write_data(0, 0x00);
	sunxi_lcd_cpu_write_data(0, 0x0f);
#if 0
	sunxi_lcd_cpu_write_index(0, 0x2B);
	sunxi_lcd_cpu_write_data(0, 0x00);
	sunxi_lcd_cpu_write_data(0, 0x00);
	sunxi_lcd_cpu_write_data(0, 0x01);
	sunxi_lcd_cpu_write_data(0, 0x3F);
#endif
	sunxi_lcd_cpu_write_index(0, 0x36);
	sunxi_lcd_cpu_write_data(0, 0x68);	//0xe8

	sunxi_lcd_cpu_write_index(0, 0x3A);
	sunxi_lcd_cpu_write_data(0, 0x55);	/* 06 ---->262K(RGB666);05---->65K(RGB565) */


#if defined(CPU_TRI_MODE)
	/* enable te, mode 0 */

	sunxi_lcd_cpu_write_index(0, 0x35);
	sunxi_lcd_cpu_write_data(0, 0x00);

#endif
	sunxi_lcd_cpu_write_index(0, 0x20);  	// Display Inversion OFF

	sunxi_lcd_cpu_write_index(0, 0x11);	// sleep out
	sunxi_lcd_delay_ms(120);
	sunxi_lcd_cpu_write_index(0, 0x29);	// display on

	sunxi_lcd_cpu_write_index(0, 0x2C); /* start memory write */
	//sunxi_lcd_cpu_write_index(0, 0x2c);
	sunxi_lcd_delay_ms(50);
}

/* sel: 0:lcd0; 1:lcd1 */
static s32 LCD_user_defined_func(u32 sel, u32 para1, u32 para2, u32 para3)
{
	return 0;
}


/* panel driver name, must mach the name of lcd_drv_name in sys_config.fex */
__lcd_panel_t ili9488_cpu_panel = {
	.name = "ili9488_cpu",
	.func = {
		.cfg_panel_info = LCD_cfg_panel_info,
		.cfg_open_flow = LCD_open_flow,
		.cfg_close_flow = LCD_close_flow,
		.lcd_user_defined_func = LCD_user_defined_func,
	},
};
