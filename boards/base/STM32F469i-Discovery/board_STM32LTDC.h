/*
 * This file is subject to the terms of the GFX License. If a copy of
 * the license was not distributed with this file, you can obtain one at:
 *
 *              http://ugfx.org/license.html
 */

#ifndef _GDISP_LLD_BOARD_H
#define _GDISP_LLD_BOARD_H

// Avoid naming collisions with CubeHAL
#undef Red
#undef Green
#undef Blue

// Don't allow the driver to init the LTDC clock. We will do it here
#define LTDC_NO_CLOCK_INIT		TRUE

//#include "stm32f4xx_hal.h"
//#include "stm32f4xx_hal_sdram.h"
//#include "stm32f4xx_hal_rcc.h"
//#include "stm32f4xx_hal_gpio.h"
//#include "stm32f4xx_hal_ltdc.h"
#include "stm32469i_discovery_lcd.h"

  LTDC_HandleTypeDef  hltdc_eval;
  static DSI_VidCfgTypeDef hdsivideo_handle;
  DSI_HandleTypeDef hdsi_eval;

// Panel parameters
// This panel is a KoD KM-040TMP-02-0621 DSI LCD Display.

static const ltdcConfig driverCfg = {
	800, 480,								// Width, Height (pixels)
	120, 12,									// Horizontal, Vertical sync (pixels)
	120, 12,									// Horizontal, Vertical back porch (pixels)
	120, 12,									// Horizontal, Vertical front porch (pixels)
	0,										// Sync flags
	0x000000,								// Clear color (RGB888)

	{										// Background layer config
		(LLDCOLOR_TYPE *)SDRAM_DEVICE_ADDR, // Frame buffer address
		800, 480,							// Width, Height (pixels)
		800 * LTDC_PIXELBYTES,				// Line pitch (bytes)
		LTDC_PIXELFORMAT,					// Pixel format
		0, 0,								// Start pixel position (x, y)
		800, 480,							// Size of virtual layer (cx, cy)
		LTDC_COLOR_FUCHSIA,					// Default color (ARGB8888)
		0x980088,							// Color key (RGB888)
		LTDC_BLEND_FIX1_FIX2,				// Blending factors
		0,									// Palette (RGB888, can be NULL)
		0,									// Palette length
		0xFF,								// Constant alpha factor
		LTDC_LEF_ENABLE						// Layer configuration flags
	},

	LTDC_UNUSED_LAYER_CONFIG				// Foreground layer config
};

/* Display timing */
#define KoD_FREQUENCY_DIVIDER 7

static GFXINLINE void init_board(GDisplay *g) {

	// As we are not using multiple displays we set g->board to NULL as we don't use it
	g->board = 0;

	  DSI_PLLInitTypeDef dsiPllInit;
  	DSI_PHY_TimerTypeDef  PhyTimings;
//  	static RCC_PeriphCLKInitTypeDef  PeriphClkInitStruct;
  	uint32_t LcdClock  = 30000;//27429; /*!< LcdClk = 27429 kHz */

  	/**
  	* @brief  Default Active LTDC Layer in which drawing is made is LTDC Layer Background
  	*/
//	static uint32_t  ActiveLayer = LTDC_ACTIVE_LAYER_BACKGROUND;

	/**
  	* @brief  Current Drawing Layer properties variable
  	*/
//	static LCD_DrawPropTypeDef DrawProp[LTDC_MAX_LAYER_NUMBER];

  	uint32_t laneByteClk_kHz = 0;
  	uint32_t                   VSA; /*!< Vertical start active time in units of lines */
  	uint32_t                   VBP; /*!< Vertical Back Porch time in units of lines */
  	uint32_t                   VFP; /*!< Vertical Front Porch time in units of lines */
  	uint32_t                   VACT; /*!< Vertical Active time in units of lines = imageSize Y in pixels to display */
  	uint32_t                   HSA; /*!< Horizontal start active time in units of lcdClk */
  	uint32_t                   HBP; /*!< Horizontal Back Porch time in units of lcdClk */
  	uint32_t                   HFP; /*!< Horizontal Front Porch time in units of lcdClk */
  	uint32_t                   HACT; /*!< Horizontal Active time in units of lcdClk = imageSize X in pixels to display */
  
  
  	/* Toggle Hardware Reset of the DSI LCD using
  	* its XRES signal (active low) */
  	BSP_LCD_Reset();

  	/* Call first MSP Initialize only in case of first initialization
  	* This will set IP blocks LTDC, DSI and DMA2D
  	* - out of reset
  	* - clocked
  	* - NVIC IRQ related to IP blocks enabled
  	*/
  	BSP_LCD_MspInit();
  
	/*************************DSI Initialization***********************************/  
  
  	/* Base address of DSI Host/Wrapper registers to be set before calling De-Init */
  	hdsi_eval.Instance = DSI;
  
  	HAL_DSI_DeInit(&(hdsi_eval));
  
	#if !defined(USE_STM32469I_DISCO_REVA)
  		dsiPllInit.PLLNDIV  = 125;
  		dsiPllInit.PLLIDF   = DSI_PLL_IN_DIV2;
  		dsiPllInit.PLLODF   = DSI_PLL_OUT_DIV1;
	#else  
  		dsiPllInit.PLLNDIV  = 100;
  		dsiPllInit.PLLIDF   = DSI_PLL_IN_DIV5;
  		dsiPllInit.PLLODF   = DSI_PLL_OUT_DIV1;
	#endif
  		laneByteClk_kHz = 62500; /* 500 MHz / 8 = 62.5 MHz = 62500 kHz */

  	/* Set number of Lanes */
  	hdsi_eval.Init.NumberOfLanes = DSI_TWO_DATA_LANES;
  
  	/* TXEscapeCkdiv = f(LaneByteClk)/15.62 = 4 */
  	hdsi_eval.Init.TXEscapeCkdiv = laneByteClk_kHz/15620; 
  
  	HAL_DSI_Init(&(hdsi_eval), &(dsiPllInit));

  	/* Timing parameters for all Video modes
  	* Set Timing parameters of LTDC depending on its chosen orientation
  	*/

  	/* lcd_orientation == LCD_ORIENTATION_LANDSCAPE */
    uint32_t lcd_x_size = OTM8009A_800X480_WIDTH;  /* 800 */
    uint32_t lcd_y_size = OTM8009A_800X480_HEIGHT; /* 480 */

  	HACT = lcd_x_size;
  	VACT = lcd_y_size;
  
  	/* The following values are same for portrait and landscape orientations */
  	VSA  = 12;//OTM8009A_480X800_VSYNC;        /* 12  */
  	VBP  = 12;//OTM8009A_480X800_VBP;          /* 12  */
  	VFP  = 12;//OTM8009A_480X800_VFP;          /* 12  */
  	HSA  = 120;//OTM8009A_480X800_HSYNC;        /* 63  */
  	HBP  = 120;//OTM8009A_480X800_HBP;          /* 120 */
  	HFP  = 120;//OTM8009A_480X800_HFP;          /* 120 */

  	hdsivideo_handle.VirtualChannelID = LCD_OTM8009A_ID;
  	hdsivideo_handle.ColorCoding = LCD_DSI_PIXEL_DATA_FMT_RBG888;
  	hdsivideo_handle.VSPolarity = DSI_VSYNC_ACTIVE_HIGH;
  	hdsivideo_handle.HSPolarity = DSI_HSYNC_ACTIVE_HIGH;
  	hdsivideo_handle.DEPolarity = DSI_DATA_ENABLE_ACTIVE_HIGH;  
  	hdsivideo_handle.Mode = DSI_VID_MODE_BURST; /* Mode Video burst ie : one LgP per line */
  	hdsivideo_handle.NullPacketSize = 0xFFF;
  	hdsivideo_handle.NumberOfChunks = 0;
  	hdsivideo_handle.PacketSize                = HACT; /* Value depending on display orientation choice portrait/landscape */ 
  	hdsivideo_handle.HorizontalSyncActive      = (HSA * laneByteClk_kHz) / LcdClock;
  	hdsivideo_handle.HorizontalBackPorch       = (HBP * laneByteClk_kHz) / LcdClock;
  	hdsivideo_handle.HorizontalLine            = ((HACT + HSA + HBP + HFP) * laneByteClk_kHz) / LcdClock; /* Value depending on display orientation choice portrait/landscape */
  	hdsivideo_handle.VerticalSyncActive        = VSA;
  	hdsivideo_handle.VerticalBackPorch         = VBP;
  	hdsivideo_handle.VerticalFrontPorch        = VFP;
  	hdsivideo_handle.VerticalActive            = VACT; /* Value depending on display orientation choice portrait/landscape */
  
  	/* Enable or disable sending LP command while streaming is active in video mode */
  	hdsivideo_handle.LPCommandEnable = DSI_LP_COMMAND_ENABLE; /* Enable sending commands in mode LP (Low Power) */
  
  	/* Largest packet size possible to transmit in LP mode in VSA, VBP, VFP regions */
  	/* Only useful when sending LP packets is allowed while streaming is active in video mode */
  	hdsivideo_handle.LPLargestPacketSize = 16;

  	/* Largest packet size possible to transmit in LP mode in HFP region during VACT period */
  	/* Only useful when sending LP packets is allowed while streaming is active in video mode */
  	hdsivideo_handle.LPVACTLargestPacketSize = 0;

  	/* Specify for each region of the video frame, if the transmission of command in LP mode is allowed in this region */
  	/* while streaming is active in video mode                                                                         */
  	hdsivideo_handle.LPHorizontalFrontPorchEnable = DSI_LP_HFP_ENABLE;   /* Allow sending LP commands during HFP period */
  	hdsivideo_handle.LPHorizontalBackPorchEnable  = DSI_LP_HBP_ENABLE;   /* Allow sending LP commands during HBP period */
  	hdsivideo_handle.LPVerticalActiveEnable = DSI_LP_VACT_ENABLE;  /* Allow sending LP commands during VACT period */
  	hdsivideo_handle.LPVerticalFrontPorchEnable = DSI_LP_VFP_ENABLE;   /* Allow sending LP commands during VFP period */
  	hdsivideo_handle.LPVerticalBackPorchEnable = DSI_LP_VBP_ENABLE;   /* Allow sending LP commands during VBP period */
  	hdsivideo_handle.LPVerticalSyncActiveEnable = DSI_LP_VSYNC_ENABLE; /* Allow sending LP commands during VSync = VSA period */
  
  	/* Configure DSI Video mode timings with settings set above */
  	HAL_DSI_ConfigVideoMode(&(hdsi_eval), &(hdsivideo_handle));

  	/* Configure DSI PHY HS2LP and LP2HS timings */
  	PhyTimings.ClockLaneHS2LPTime = 35;
  	PhyTimings.ClockLaneLP2HSTime = 35;
  	PhyTimings.DataLaneHS2LPTime = 35;
  	PhyTimings.DataLaneLP2HSTime = 35;
  	PhyTimings.DataLaneMaxReadTime = 0;
  	PhyTimings.StopWaitTime = 10;
  	HAL_DSI_ConfigPhyTimer(&hdsi_eval, &PhyTimings);

	/*************************End DSI Initialization*******************************/ 

  /************************LTDC Initialization***********************************/

	// KoD LCD clock configuration
	// PLLSAI_VCO Input = HSE_VALUE/PLL_M = 1 Mhz
	// PLLSAI_VCO Output = PLLSAI_VCO Input * PLLSAIN = 384 Mhz
	// PLLLCDCLK = PLLSAI_VCO Output/PLLSAIR = 384/7 = 54.857 Mhz
	// LTDC clock frequency = PLLLCDCLK / LTDC_PLLSAI_DIVR_2 = 54.857/2 = 27.429Mhz
  #define STM32_SAISRC_NOCLOCK    (0 << 23)   /**< No clock.                  */
  #define STM32_SAISRC_PLL        (1 << 23)   /**< SAI_CKIN is PLL.           */
  #define STM32_SAIR_DIV2         (0 << 16)   /**< R divided by 2.            */
  #define STM32_SAIR_DIV4         (1 << 16)   /**< R divided by 4.            */
  #define STM32_SAIR_DIV8         (2 << 16)   /**< R divided by 8.            */
  #define STM32_SAIR_DIV16        (3 << 16)   /**< R divided by 16.           */

	#define STM32_PLLSAIN_VALUE                 384
  #define STM32_PLLSAIQ_VALUE                 4
	#define STM32_PLLSAIR_VALUE                 KoD_FREQUENCY_DIVIDER
	#define STM32_PLLSAIR_POST                  STM32_SAIR_DIV2

  RCC->PLLSAICFGR = (STM32_PLLSAIN_VALUE << 6) | (STM32_PLLSAIR_VALUE << 28) | (STM32_PLLSAIQ_VALUE << 24);
  RCC->DCKCFGR = (RCC->DCKCFGR & ~RCC_DCKCFGR_PLLSAIDIVR) | STM32_PLLSAIR_POST;
  RCC->CR |= RCC_CR_PLLSAION;

//	PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_LTDC;
//  PeriphClkInitStruct.PLLSAI.PLLSAIN = 384;
//  PeriphClkInitStruct.PLLSAI.PLLSAIR = 7;
//  PeriphClkInitStruct.PLLSAIDivR = RCC_PLLSAIDIVR_2;
//  HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);

    /* Get LTDC Configuration from DSI Configuration */
//    HAL_LTDC_StructInitFromVideoConfig(&(hltdc_eval), &(hdsivideo_handle));

    /* Initialize the LTDC */
//    HAL_LTDC_Init(&hltdc_eval);

  /* Enable the DSI host and wrapper : but LTDC is not started yet at this stage */
  HAL_DSI_Start(&(hdsi_eval));

  #if !defined(DATA_IN_ExtSDRAM)
    /* Initialize the SDRAM */
    BSP_SDRAM_Init();
  #endif /* DATA_IN_ExtSDRAM */
}

static GFXINLINE void post_init_board(GDisplay* g)
{
	(void)g;
  
  	/* Initialize the font */
  	BSP_LCD_SetFont(&LCD_DEFAULT_FONT);
  
	/************************End LTDC Initialization*******************************/
		
		
	/***********************OTM8009A Initialization********************************/  
  
  	/* Initialize the OTM8009A LCD Display IC Driver (KoD LCD IC Driver)
  	*  depending on configuration set in 'hdsivideo_handle'.
  	*/
  	OTM8009A_Init(OTM8009A_FORMAT_RGB888, OTM8009A_ORIENTATION_LANDSCAPE);
  
	/***********************End OTM8009A Initialization****************************/ 

	// ------------------------------------------------------------------------

	//BSP_LCD_LayerDefaultInit(LTDC_ACTIVE_LAYER_BACKGROUND, LCD_FB_START_ADDRESS);
	//BSP_LCD_SelectLayer(LTDC_ACTIVE_LAYER_BACKGROUND);
	//BSP_LCD_SetLayerVisible(LTDC_ACTIVE_LAYER_FOREGROUND, DISABLE);
}

static GFXINLINE void set_backlight(GDisplay* g, uint8_t percent)
{
	(void)g;
	(void)percent;
}

#endif /* _GDISP_LLD_BOARD_H */
