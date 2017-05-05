/*
 * This file is subject to the terms of the GFX License. If a copy of
 * the license was not distributed with this file, you can obtain one at:
 *
 *              http://ugfx.org/license.html
 */

#ifndef _GINPUT_LLD_MOUSE_BOARD_H
#define _GINPUT_LLD_MOUSE_BOARD_H

#include "tm_stm32_i2c.h"
#include "stm32469i_discovery_ts.h"
#include "ft6x06.h"

// Resolution and Accuracy Settings
#define GMOUSE_FT6x06_PEN_CALIBRATE_ERROR		8
#define GMOUSE_FT6x06_PEN_CLICK_ERROR			6
#define GMOUSE_FT6x06_PEN_MOVE_ERROR			4
#define GMOUSE_FT6x06_FINGER_CALIBRATE_ERROR	14
#define GMOUSE_FT6x06_FINGER_CLICK_ERROR		18
#define GMOUSE_FT6x06_FINGER_MOVE_ERROR			14

// How much extra data to allocate at the end of the GMouse structure for the board's use
#define GMOUSE_FT6x06_BOARD_DATA_SIZE			0

// Set this to TRUE if you want self-calibration.
//	NOTE:	This is not as accurate as real calibration.
//			It requires the orientation of the touch panel to match the display.
//			It requires the active area of the touch panel to exactly match the display size.
//#define GMOUSE_FT6x06_SELF_CALIBRATE			TRUE

#define FT6x06_SLAVE_ADDR 0x54

static bool_t init_board(GMouse* m, unsigned driverinstance) {
	(void)m;
	
	TS_StateTypeDef ts_state;
	
	return TS_OK == BSP_TS_Init(BSP_LCD_GetXSize(), BSP_LCD_GetYSize());
}

#define aquire_bus(m)
#define release_bus(m);

static void write_reg(GMouse* m, uint8_t reg, uint8_t val) {
	(void)m;

	//TM_I2C_Write(I2C1, FT6x06_SLAVE_ADDR, reg, val);
}

static uint8_t read_byte(GMouse* m, uint8_t reg) {
	(void)m;
	
  /*uint8_t data;
	TM_I2C_Read(I2C1, FT6x06_SLAVE_ADDR, reg, &data);
	return data;*/
	TS_StateTypeDef ts_state;
	if(reg == FT6x06_TOUCH_POINTS){
		if (TS_OK != BSP_TS_GetState(&ts_state))
    {
        return FALSE;
    }
		return ts_state.touchDetected;
	}
}

static uint16_t read_word(GMouse* m, uint8_t reg) {
	(void)m;
  
	/*uint8_t data[2];
	TM_I2C_ReadMulti(I2C1, FT6x06_SLAVE_ADDR, reg, data, 2);
	return (data[1]<<8 | data[0]);*/
	
	TS_StateTypeDef ts_state;
	if (TS_OK == BSP_TS_GetState(&ts_state))
	{
		if(reg == FT6x06_TOUCH1_XH){
			return (coord_t)ts_state.touchX[0];
		}else if(reg == FT6x06_TOUCH1_YH){
			return (coord_t)ts_state.touchY[0];
		}
	}
}

#endif /* _GINPUT_LLD_MOUSE_BOARD_H */
