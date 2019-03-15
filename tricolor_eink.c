/**
  *@file tricolor_eink.c
  *@brief driver for tri color e-ink display
  *@author Jason Berger
  *@date 3/14/2019
  */

#include "tricolor_eink.h"
#include "string.h"
#include <stdlib.h>

const unsigned char lut_vcom0[] = {
    0x0E, 0x14, 0x01, 0x0A, 0x06, 0x04, 0x0A, 0x0A,
    0x0F, 0x03, 0x03, 0x0C, 0x06, 0x0A, 0x00
};

const unsigned char lut_w[] = {
    0x0E, 0x14, 0x01, 0x0A, 0x46, 0x04, 0x8A, 0x4A,
    0x0F, 0x83, 0x43, 0x0C, 0x86, 0x0A, 0x04
};

const unsigned char lut_b[] = {
    0x0E, 0x14, 0x01, 0x8A, 0x06, 0x04, 0x8A, 0x4A,
    0x0F, 0x83, 0x43, 0x0C, 0x06, 0x4A, 0x04
};

const unsigned char lut_g1[] = {
    0x8E, 0x94, 0x01, 0x8A, 0x06, 0x04, 0x8A, 0x4A,
    0x0F, 0x83, 0x43, 0x0C, 0x06, 0x0A, 0x04
};

const unsigned char lut_g2[] = {
    0x8E, 0x94, 0x01, 0x8A, 0x06, 0x04, 0x8A, 0x4A,
    0x0F, 0x83, 0x43, 0x0C, 0x06, 0x0A, 0x04
};

const unsigned char lut_vcom1[] = {
    0x03, 0x1D, 0x01, 0x01, 0x08, 0x23, 0x37, 0x37,
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const unsigned char lut_red0[] = {
    0x83, 0x5D, 0x01, 0x81, 0x48, 0x23, 0x77, 0x77,
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const unsigned char lut_red1[] = {
    0x03, 0x1D, 0x01, 0x01, 0x08, 0x23, 0x37, 0x37,
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};


static void tri_eink_set_luts_bw(tri_eink_t* dev);
static void tri_eink_set_luts_red(tri_eink_t* dev);


mrt_status_t tri_eink_init(tri_eink_t* dev, tri_eink_hw_cfg_t* hw, int width, int height)
{

    //copy in hardware config
    memcpy(&dev->mHW, hw, sizeof(tri_eink_hw_cfg_t));


    //initialize graphics canvases as buffered so they maintain their own data
    mono_gfx_init_buffered(dev->mCanvasBlk, width, height);
    mono_gfx_init_buffered(dev->mCanvasRed, width, height);

    dev->mFont = NULL;

    tri_eink_reset(dev) ;

    tri_eink_cmd(dev,INK_POWER_SETTING);
    tri_eink_data(dev,0x07);
    tri_eink_data(dev,0x00);
    tri_eink_data(dev,0x08);
    tri_eink_data(dev,0x00);
    tri_eink_cmd(dev,INK_BOOSTER_SOFT_START);
    tri_eink_data(dev,0x07);
    tri_eink_data(dev,0x07);
    tri_eink_data(dev,0x07);
    tri_eink_cmd(dev,INK_POWER_ON);

    tri_eink_wait(dev, 100 );

    tri_eink_cmd(dev,INK_PANEL_SETTING);
    tri_eink_data(dev,0xcf);
    tri_eink_cmd(dev,INK_VCOM_AND_DATA_INTERVAL_SETTING);
    tri_eink_data(dev,0xF0);
    tri_eink_cmd(dev,INK_PLL_CONTROL);
    tri_eink_data(dev,0x39);
    tri_eink_cmd(dev,INK_TCON_RESOLUTION);  //set x and y
    tri_eink_data(dev,0xC8);            //x
    tri_eink_data(dev,0x00);            //y High eight
    tri_eink_data(dev,0xC8);            //y Low eight
    tri_eink_cmd(dev,INK_VCM_DC_SETTING_REGISTER); //VCOM
    tri_eink_data(dev,0x0E);

    tri_eink_set_luts_bw(dev);
    tri_eink_set_luts_red(dev);

  return MRT_STATUS_OK;
}


void tri_eink_cmd(tri_eink_t* dev, uint8_t cmd)
{

  MRT_GPIO_WRITE(dev->mHW.mDC,LOW);
  MRT_GPIO_WRITE(dev->mHW.mCS,LOW);
  MRT_SPI_TRANSMIT(dev->mHW.mSpi , &cmd, 1, 20);
  MRT_GPIO_WRITE(dev->mHW.mCS,HIGH);

}

void tri_eink_data(tri_eink_t* dev, uint8_t data)
{
  MRT_GPIO_WRITE(dev->mHW.mDC,HIGH);
  MRT_GPIO_WRITE(dev->mHW.mCS,LOW);
  MRT_SPI_TRANSMIT(dev->mHW.mSpi , &data, 1, 20);
  MRT_GPIO_WRITE(dev->mHW.mCS,HIGH);


}


mrt_status_t tri_eink_update(tri_eink_t* dev)
{
    uint8_t Temp = 0x00;
    uint16_t Width, Height;

    //both canvases are the same size...
    Width = (dev->mCanvasBlk.mWidth % 8 == 0)? (dev->mCanvasBlk.mWidth / 8 ): (dev->mCanvasBlk.mWidth / 8 + 1);
    Height = dev->mCanvasBlk.mHeight;

    uint8_t* blackimage= dev->mCanvasBlk.mBuffer;
    uint8_t* redimage = dev->mCanvasRed.mBuffer;

    tri_eink_cmd(dev,INK_DATA_START_TRANSMISSION_1);
    for (uint16_t j = 0; j < Height; j++) {
        for (uint16_t i = 0; i < Width; i++) {
            Temp = 0x00;
            for (int bit = 0; bit < 4; bit++) {
                if ((blackimage[i + j * Width] & (0x80 >> bit)) != 0) {
                    Temp |= 0xC0 >> (bit * 2);
                }
            }
            tri_eink_data(dev,Temp);
            Temp = 0x00;
            for (int bit = 4; bit < 8; bit++) {
                if ((blackimage[i + j * Width] & (0x80 >> bit)) != 0) {
                    Temp |= 0xC0 >> ((bit - 4) * 2);
                }
            }
            tri_eink_data(dev,Temp);
        }
    }
    MRT_DELAY_MS(2);

    tri_eink_cmd(dev,INK_DATA_START_TRANSMISSION_2);
    for (uint16_t j = 0; j < Height; j++) {
        for (uint16_t i = 0; i < Width; i++) {
            tri_eink_data(dev,redimage[i + j * Width]);
        }
    }
    MRT_DELAY_MS(2);

    //Display refresh
    tri_eink_cmd(dev,INK_DISPLAY_REFRESH);
    tri_eink_wait(dev, 100 );

  return MRT_STATUS_OK;
}



mrt_status_t tri_eink_reset(tri_eink_t* dev)
{
  MRT_GPIO_WRITE(dev->mHW.mRST, HIGH);
  MRT_DELAY_MS(200);
  MRT_GPIO_WRITE(dev->mHW.mRST, LOW);
  MRT_DELAY_MS(200);
  MRT_GPIO_WRITE(dev->mHW.mRST, HIGH);
  MRT_DELAY_MS(200);

  return MRT_STATUS_OK;
}

mrt_status_t tri_eink_draw_bmp(tri_eink_t* dev, uint16_t x, uint16_t y, GFXBmp* bmp, ink_color_e color)
{
  mrt_status_t status;
  switch(color)
  {
    case COLOR_BLACK:
      status = mono_gfx_draw_bmp(dev->mCanvasBlk, x,y,bmp);
      break;
    case COLOR_RED:
      status = mono_gfx_draw_bmp(dev->mCanvasRed, x,y,bmp);
      break;
    default:
      status = MRT_STATUS_ERROR;
  }

  return status;
}


mrt_status_t tri_eink_print(tri_eink_t* dev, uint16_t x, uint16_t y, const char * text, ink_color_e color)
{
  mrt_status_t status;
  switch(color)
  {
    case COLOR_BLACK:
      dev->mCanvasBlk.mFont = dev->mFont;
      status = mono_gfx_print(dev->mCanvasBlk, x,y,text);
      break;
    case COLOR_RED:
      dev->mCanvasRed.mFont = dev->mFont;
      status = mono_gfx_print(dev->mCanvasRed, x,y,text);
      break;
    default:
      status = MRT_STATUS_ERROR;
  }

  return status;
}


mrt_status_t tri_eink_fill(tri_eink_t* dev, ink_color_e color)
{
  mrt_status_t status;
  switch(color)
  {
    case COLOR_BLACK:
      status = mono_gfx_fill(dev->mCanvasBlk);
      break;
    case COLOR_RED:
      status = mono_gfx_fill(dev->mCanvasRed);
      break;
    default:
      status = MRT_STATUS_ERROR;
  }

  return status;
}


mrt_status_t tri_eink_wait(tri_eink_t* dev, int timeout_ms)
{
	uint8_t ready;

	while(timeout_ms > 0)
	{

		ready = MRT_GPIO_READ(dev->mHW.mBusy);
		if(ready)
		{
			return MRT_STATUS_OK;
		}

		MRT_DELAY_MS(1);
		timeout_ms --;
	}


  return MRT_STATUS_ERROR;
}


mrt_status_t tri_eink_sleep(tri_eink_t* dev)
{
  tri_eink_cmd(dev,INK_VCOM_AND_DATA_INTERVAL_SETTING);
  tri_eink_data(dev,0x17);
  tri_eink_cmd(dev,INK_VCM_DC_SETTING_REGISTER);         //to solve Vcom drop
  tri_eink_data(dev,0x00);
  tri_eink_cmd(dev,INK_POWER_SETTING);         //power setting
  tri_eink_data(dev,0x02);        //gate switch to external
  tri_eink_data(dev,0x00);
  tri_eink_data(dev,0x00);
  tri_eink_data(dev,0x00);
  tri_eink_wait(dev, 100 );
  tri_eink_cmd(dev,INK_POWER_OFF);         //power off

  return MRT_STATUS_OK;
}


/**
  *@brief sets the look up tables for black/white
  */
static void tri_eink_set_luts_bw(tri_eink_t* dev)
{

  uint16_t count;
    tri_eink_cmd(dev,0x20);         //g vcom
    for(count = 0; count < 15; count++) {
        tri_eink_data(dev,lut_vcom0[count]);
    }
    tri_eink_cmd(dev,0x21);        //g ww --
    for(count = 0; count < 15; count++) {
        tri_eink_data(dev,lut_w[count]);
    }
    tri_eink_cmd(dev,0x22);         //g bw r
    for(count = 0; count < 15; count++) {
        tri_eink_data(dev,lut_b[count]);
    }
    tri_eink_cmd(dev,0x23);         //g wb w
    for(count = 0; count < 15; count++) {
        tri_eink_data(dev,lut_g1[count]);
    }
    tri_eink_cmd(dev,0x24);         //g bb b
    for(count = 0; count < 15; count++) {
        tri_eink_data(dev,lut_g2[count]);
    }
}

/**
  *@brief sets the look up tables for red
  */
static void tri_eink_set_luts_red(tri_eink_t* dev)
{
  uint16_t count;
  tri_eink_cmd(dev,0x25);
  for(count = 0; count < 15; count++) {
      tri_eink_data(dev,lut_vcom1[count]);
  }
  tri_eink_cmd(dev,0x26);
  for(count = 0; count < 15; count++) {
      tri_eink_data(dev,lut_red0[count]);
  }
  tri_eink_cmd(dev,0x27);
  for(count = 0; count < 15; count++) {
      tri_eink_data(dev,lut_red1[count]);
  }
}
