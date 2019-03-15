/**
  *@file tricolor_eink.h
  *@brief driver for tri color e-ink display
  *@author Jason Berger
  *@date )3/14/2019
  */

#include "Platforms/Common/mrt_platform.h"
#include "Utilities/GFX/Fonts/gfxfont.h"

// EPD1IN54B commands
#define INK_PANEL_SETTING                               0x00
#define INK_POWER_SETTING                               0x01
#define INK_POWER_OFF                                   0x02
#define INK_POWER_OFF_SEQUENCE_SETTING                  0x03
#define INK_POWER_ON                                    0x04
#define INK_POWER_ON_MEASURE                            0x05
#define INK_BOOSTER_SOFT_START                          0x06
#define INK_DEEP_SLEEP                                  0x07
#define INK_DATA_START_TRANSMISSION_1                   0x10
#define INK_DATA_STOP                                   0x11
#define INK_DISPLAY_REFRESH                             0x12
#define INK_DATA_START_TRANSMISSION_2                   0x13
#define INK_PLL_CONTROL                                 0x30
#define INK_TEMPERATURE_SENSOR_COMMAND                  0x40
#define INK_TEMPERATURE_SENSOR_CALIBRATION              0x41
#define INK_TEMPERATURE_SENSOR_WRITE                    0x42
#define INK_TEMPERATURE_SENSOR_READ                     0x43
#define INK_VCOM_AND_DATA_INTERVAL_SETTING              0x50
#define INK_LOW_POWER_DETECTION                         0x51
#define INK_TCON_SETTING                                0x60
#define INK_TCON_RESOLUTION                             0x61
#define INK_SOURCE_AND_GATE_START_SETTING               0x62
#define INK_GET_STATUS                                  0x71
#define INK_AUTO_MEASURE_VCOM                           0x80
#define INK_VCOM_VALUE                                  0x81
#define INK_VCM_DC_SETTING_REGISTER                     0x82
#define INK_PROGRAM_MODE                                0xA0
#define INK_ACTIVE_PROGRAM                              0xA1
#define INK_READ_OTP_DATA                               0xA2



typedef enum{
  COLOR_NONE,
  COLOR_BLACK,
  COLOR_RED
}ink_color_e;

typedef struct{
  mrt_spi_handle_t mSpi;  //handle for spi bus
  mrt_gpio_t mCS;         //gpio for chip select
  mrt_gpio_t mDC;         //Data/Command mode pin
  mrt_gpio_t mRST;        //Reset (active low )
  mrt_gpio_t mBusy;       //signal to indicate display is busy
}tri_eink_hw_cfg_t;

typedef struct{
  uint8_t* mBufferRed;
  uint8_t* mBufferBlk;
  int mWidth;                   //width of display in pixels
  int mHeight;                  //height of display in pixels
  GFXfont* mFont;               //font to use for printing
  uint8_t* mBuffer;             //buffer of pixel data
  uint32_t mBufferSize;
  tri_eink_hw_cfg_t mHW;
}tri_eink_t;

/**
  *@brief initialize lcd driver
  *@param dev ptr to device descriptor
  *@param hw tri_eink hardware config
  *@param width width (in pixels) of display buffer
  *@param height height (in pixels) of display buffer
  *@return MRT_STATUS_OK
  */
mrt_status_t tri_eink_init(tri_eink_t* dev, tri_eink_hw_cfg_t* hw, int width, int height);

/**
  *@brief sends command over spi bus to device
  *@param dev ptr to tri_eink device
  *@param cmd command to send
  */
void tri_eink_cmd(tri_eink_t* dev, uint8_t cmd);

/**
  *@brief sends data byte over spi bus to device
  *@param dev ptr to tri_eink device
  *@param cmd command to send
  */
void tri_eink_data(tri_eink_t* dev, uint8_t data);

/**
  *@brief updates the device with the local buffer
  *@param dev ptr to device descriptor
  *@return MRT_STATUS_OK
  */
mrt_status_t tri_eink_update(tri_eink_t* dev);


/**
  *@brief performs reset of the device by setting reset pin
  *@param dev ptr to device descriptor
  *@return status of operation
  */
mrt_status_t tri_eink_reset(tri_eink_t* dev);


/**
  *@brief writes an array of bytes to the buffer
  *@param dev ptr to device descriptor
  *@param data ptr to black data being written
  *@param len number of bytes being written
  *@param wrap whether or not to wrap when we reach the end of current row
  *@return status of operation
  */
mrt_status_t tri_eink_write_buffer(tri_eink_t* dev, uint8_t* data, int len, ink_color_e color,  bool wrap);

/**
  *@brief Draws a bitmap to the buffer
  *@param dev ptr to device descriptor
  *@param x x coord to begin drawing at
  *@param y y coord to begin drawing at
  *@param bmp bitmap to draw
  *@return status of operation
  */
mrt_status_t tri_eink_draw_bmp(tri_eink_t* dev, uint16_t x, uint16_t y, GFXBmp* bmpBlk, ink_color_e color);

/**
  *@brief Draws rendered text to the buffer
  *@param dev ptr to device descriptor
  *@param x x coord to begin drawing at
  *@param y y coord to begin drawing at
  *@param text text to be written
  *@return status of operation
  */
mrt_status_t tri_eink_print(tri_eink_t* dev, uint16_t x, uint16_t y, const char * text, ink_color_e color);

/**
  *@brief fill buffer with value
  *@param dev ptr to device
  *@param val value to write
  *@return status of operation
  */
mrt_status_t tri_eink_fill(tri_eink_t* dev, ink_color_e color);

/**
  *@brief waits for device to not be busy
  *@param dev ptr to device
  *@return status of operation
  */
mrt_status_t tri_eink_wait(tri_eink_t* dev, uint32_t timeout_ms);

/**
  *@brief puts device into sleep mode
  *@param dev ptr to device
  *@return status
  */
mrt_status_t tri_eink_sleep(tri_eink_t* dev);
