/**
  *@file tricolor_eink.c
  *@brief driver for tri color e-ink display
  *@author Jason Berger
  *@date 3/14/2019
  */

#include "tricolor_eink.h"

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


static void tri_eink_set_luts_bw(void);
static void tri_eink_set_luts_red(void);


mrt_status_t tri_eink_init(tri_eink_t* dev, tri_eink_hw_cfg_t* hw, int width, int height)
{
    memcpy(dev->mHW, hw, sizeof(tri_eink_hw_cfg_t));

    dev->mBufferSize = (width * height)/8;
    dev->mBufferRed = (uint8_t*)malloc( dev->mBufferSize);
    dev->mBufferBlk = (uint8_t*)malloc( dev->mBufferSize);
    dev->mWidth = width;
    dev->mHeight = height;
    dev->mFont  = NULL;


    tri_eink_reset(dev) ;

    tri_eink_cmd(dev,POWER_SETTING);
    tri_eink_data(dev,0x07);
    tri_eink_data(dev,0x00);
    tri_eink_data(dev,0x08);
    tri_eink_data(dev,0x00);
    tri_eink_cmd(dev,BOOSTER_SOFT_START);
    tri_eink_data(dev,0x07);
    tri_eink_data(dev,0x07);
    tri_eink_data(dev,0x07);
    tri_eink_cmd(dev,POWER_ON);

    tri_eink_wait(dev );

    tri_eink_cmd(dev,PANEL_SETTING);
    tri_eink_data(dev,0xcf);
    tri_eink_cmd(dev,VCOM_AND_DATA_INTERVAL_SETTING);
    tri_eink_data(dev,0xF0);
    tri_eink_cmd(dev,PLL_CONTROL);
    tri_eink_data(dev,0x39);
    tri_eink_cmd(dev,TCON_RESOLUTION);  //set x and y
    tri_eink_data(dev,0xC8);            //x
    tri_eink_data(dev,0x00);            //y High eight
    tri_eink_data(dev,0xC8);            //y Low eight
    tri_eink_cmd(dev,VCM_DC_SETTING_REGISTER); //VCOM
    tri_eink_data(dev,0x0E);

    tri_eink_set_luts_bw(void);
    tri_eink_set_luts_red(void);

  return MRT_STATUS_OK;
}


void tri_eink_cmd(tri_eink_t* dev, uint8_t cmd)
{

  MRT_GPIO_WRITE(dev->mHW.mDC,LOW);
  MRT_GPIO_WRITE(dev->mHW.mCS,LOW);
  MRT_SPI_TRANSMIT(dev->mHW.mSpi , &cmd, 1, 20);
  MRT_GPIO_WRITE(dev->mHW.mCS,HIGH);

  return MRT_STATUS_OK;
}

void tri_eink_data(tri_eink_t* dev, uint8_t data)
{
  MRT_GPIO_WRITE(dev->mHW.mDC,HIGH);
  MRT_GPIO_WRITE(dev->mHW.mCS,LOW);
  MRT_SPI_TRANSMIT(dev->mHW.mSpi , &data, 1, 20);
  MRT_GPIO_WRITE(dev->mHW.mCS,HIGH);

  return MRT_STATUS_OK;
}


mrt_status_t tri_eink_update(tri_eink_t* dev)
{
    uint8_t Temp = 0x00;
    uint16_t Width, Height;
    Width = (EPD_WIDTH % 8 == 0)? (EPD_WIDTH / 8 ): (EPD_WIDTH / 8 + 1);
    Height = EPD_HEIGHT;

    tri_eink_cmd(dev,DATA_START_TRANSMISSION_1);
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

    tri_eink_cmd(dev,DATA_START_TRANSMISSION_2);
    for (uint16_t j = 0; j < Height; j++) {
        for (uint16_t i = 0; i < Width; i++) {
            tri_eink_data(dev,redimage[i + j * Width]);
        }
    }
    MRT_DELAY_MS(2);

    //Display refresh
    tri_eink_cmd(dev,DISPLAY_REFRESH);
    tri_eink_wait(dev );

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


mrt_status_t tri_eink_write_buffer(tri_eink_t* dev, uint8_t* data, int len, ink_color_e color,  bool wrap)
{
  uint32_t cursor = (y * dev->mWidth) + x;
  uint8_t* pBuffer = dev->mBufferBlk;

  if(color == COLOR_RED)
    pBuffer = dev->mBufferRed;

  //get number of bits off of alignment in case we are not writing on a byte boundary
  uint32_t byteOffset = (cursor  / 8);
  uint8_t bitOffset = cursor % 8;

  //get number of bytes before we would wrap to next row
  int nextRow = (dev->mWidth - (cursor % dev->mWidth));
  if((nextRow < len) && (wrap == false))
  {
    len = nextRow;
  }


  uint8_t prevByte; //used for shifting in data when not aligned
  uint8_t mask;


  //If we are byte aligned , just memcpy the data in
  if(bitOffset == 0)
  {
    memcpy(&pBuffer[byteOffset], data, len);
  }
  //If we are not byte aligned, we have to mask and shift in data
  else
  {
    mask = 0xFF << (8-bitOffset);
    prevByte = pBuffer[byteOffset] & mask;

    for(int i=0; i < len; i++)
    {
      pBuffer[byteOffset++] = prevByte | (data[i] >> bitOffset);
      prevByte = data[i] << (8-bitOffset);

      if(byteOffset >= dev->mBufferSize)
        byteOffset = 0;
    }
  }


  //advance cursor
  cursor += len;

  // If its gone over, wrap
  while(cursor >= (dev->mWidth * dev->mHeight))
    cursor -=  (dev->mWidth * dev->mHeight);

  return MRT_STATUS_OK;
}


mrt_status_t tri_eink_draw_bmp(tri_eink_t* dev, uint16_t x, uint16_t y, GFXBmp* bmp, ink_color_e color )
{
  uint32_t bmpIdx = 0;
  for(int i=0; i < bmp->height; i ++)
  {
    tricolor_eink_write_buffer(dev, &bmp->data[bmpIdx], bmp->width, color, false);
    bmpIdx += bmp->width;
  }
  return MRT_STATUS_OK;
}


mrt_status_t tri_eink_print(tri_eink_t* dev, uint16_t x, uint16_t y, const char * text, ink_color_e color)
{

  //if a font has not been set, return error
  if(dev->mFont == NULL)
    return MRT_STATUS_ERROR;

  uint16_t xx =x;     //current position for writing
  uint16_t yy = y;
  GFXglyph* glyph;    //pointer to glyph for current character
  GFXBmp bmp;         //bitmap struct used to draw glyph
  char c = *text++;   //grab first character from string

  //run until we hit a null character (end of string)
  while(c != 0)
  {
    if(c == '\n')
    {
      //if character is newline, we advance the y, and reset x
      yy+= dev->mFont->yAdvance;
      xx = x;
    }
    else if((c >= dev->mFont->first) && (c <= dev->mFont->last))// make sure the font contains this character
    {
      //grab the glyph for current character from our font
      glyph = &dev->mFont->glyph[c - dev->mFont->first]; //index in glyph array is offset by first printable char in font

      //map glyph to a bitmap that we can draw
      bmp.data = &dev->mFont->bitmap[glyph->bitmapOffset];
      bmp.width = glyph->width;
      bmp.height = glyph->height;

      //draw the character
      tricolor_eink_draw_bmp(dev, xx + glyph->xOffset, yy + glyph->yOffset, &bmp );
      xx += glyph->xOffset + glyph->xAdvance;
    }


    //get next character
    c = *text++;
  }

  return MRT_STATUS_OK;
}


mrt_status_t tri_eink_fill(tri_eink_t* dev, uint8_t val)
{
  return MRT_STATUS_OK;
}

mrt_status_t tri_eink_wait(tri_eink_t* dev, uint32_t timeout_ms)
{

    while(1) {      //LOW: busy, HIGH: idle
      if(MRT_GPIO_READ(dev->mHW.mBusy) == 1)
          break;
  }
  MRT_DELAY_MS(200);

  return MRT_STATUS_OK;
}


mrt_status_t tri_eink_sleep(tri_eink_t* dev)
{
  tri_eink_cmd(dev,VCOM_AND_DATA_INTERVAL_SETTING);
  tri_eink_data(dev,0x17);
  tri_eink_cmd(dev,VCM_DC_SETTING_REGISTER);         //to solve Vcom drop
  tri_eink_data(dev,0x00);
  tri_eink_cmd(dev,POWER_SETTING);         //power setting
  tri_eink_data(dev,0x02);        //gate switch to external
  tri_eink_data(dev,0x00);
  tri_eink_data(dev,0x00);
  tri_eink_data(dev,0x00);
  tri_eink_wait(dev );
  tri_eink_cmd(dev,POWER_OFF);         //power off

  return MRT_STATUS_OK;
}


/**
  *@brief sets the look up tables for black/white
  */
static void tri_eink_set_luts_bw(void)
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
static void tri_eink_set_luts_red(void)
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
