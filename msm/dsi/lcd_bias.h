/*
 * lct_power.h - lcd bias Driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef _LCD_BIAS_H_
#define _LCD_BIAS_H_

void lcd_bias_i2c_init(void);
void lcd_bias_i2c_exit(void);
inline int set_lcd_bias_by_index(bool power);

#endif
