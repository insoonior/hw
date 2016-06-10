/**
 * @file ST7565.c
 * 
 */

/*********************
 *      INCLUDES
 *********************/
#include "hw_conf.h"
#if USE_ST7565 != 0

#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "hw/per/spi.h"
#include "hw/per/io.h"
#include "hw/per/tick.h"
#include "ST7565.h"


/*********************
 *      DEFINES
 *********************/
#define ST7565_BAUD      2000000    /*< 2,5 MHz (400 ns)*/

#define ST7565_CMD_MODE  0
#define ST7565_DATA_MODE 1

#define ST7565_HOR_RES  128
#define ST7565_VER_RES  64

#define CMD_DISPLAY_OFF         0xAE
#define CMD_DISPLAY_ON          0xAF

#define CMD_SET_DISP_START_LINE 0x40
#define CMD_SET_PAGE            0xB0

#define CMD_SET_COLUMN_UPPER    0x10
#define CMD_SET_COLUMN_LOWER    0x00

#define CMD_SET_ADC_NORMAL      0xA0
#define CMD_SET_ADC_REVERSE     0xA1

#define CMD_SET_DISP_NORMAL     0xA6
#define CMD_SET_DISP_REVERSE    0xA7

#define CMD_SET_ALLPTS_NORMAL   0xA4
#define CMD_SET_ALLPTS_ON       0xA5
#define CMD_SET_BIAS_9          0xA2
#define CMD_SET_BIAS_7          0xA3

#define CMD_RMW                 0xE0
#define CMD_RMW_CLEAR           0xEE
#define CMD_INTERNAL_RESET      0xE2
#define CMD_SET_COM_NORMAL      0xC0
#define CMD_SET_COM_REVERSE     0xC8
#define CMD_SET_POWER_CONTROL   0x28
#define CMD_SET_RESISTOR_RATIO  0x20
#define CMD_SET_VOLUME_FIRST    0x81
#define CMD_SET_VOLUME_SECOND   0x00
#define CMD_SET_STATIC_OFF      0xAC
#define CMD_SET_STATIC_ON       0xAD
#define CMD_SET_STATIC_REG      0x00
#define CMD_SET_BOOSTER_FIRST   0xF8
#define CMD_SET_BOOSTER_234     0x00
#define CMD_SET_BOOSTER_5       0x01
#define CMD_SET_BOOSTER_6       0x03
#define CMD_NOP                 0xE3
#define CMD_TEST                0xF0

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void st7565_flush(int32_t x1, int32_t y1, int32_t x2, int32_t y2);
static void st7565_command(uint8_t cmd);
static void st7565_data(uint8_t data);

/**********************
 *  STATIC VARIABLES
 **********************/
static uint8_t lcd_fb[ST7565_HOR_RES * ST7565_VER_RES / 8] = {0xAA, 0xAA};
static uint8_t pagemap[] = { 7, 6, 5, 4, 3, 2, 1, 0 };
static int32_t last_x1;
static int32_t last_y1;
static int32_t last_x2;
static int32_t last_y2;



/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * Initialize the ST7565
 */
void st7565_init(void)
{
    io_set_pin_dir(ST7565_RST_PORT,ST7565_RST_PIN, IO_DIR_OUT);
    io_set_pin_dir(ST7565_RS_PORT,ST7565_RS_PIN, IO_DIR_OUT);
    io_set_pin(ST7565_RST_PORT,ST7565_RST_PIN, 1);   
    tick_wait_ms(10);   
    io_set_pin(ST7565_RST_PORT,ST7565_RST_PIN, 0);   
    tick_wait_ms(10);   
    io_set_pin(ST7565_RST_PORT,ST7565_RST_PIN, 1);   
    tick_wait_ms(10);
    
    spi_set_baud(ST7565_DRV, ST7565_BAUD);
    
    spi_cs_en(ST7565_DRV);
    
    st7565_command(CMD_SET_BIAS_7);
    st7565_command(CMD_SET_ADC_NORMAL);
    st7565_command(CMD_SET_COM_NORMAL);
    st7565_command(CMD_SET_DISP_START_LINE);
    st7565_command(CMD_SET_POWER_CONTROL | 0x4);
    tick_wait_ms(50);

    st7565_command(CMD_SET_POWER_CONTROL | 0x6);
    tick_wait_ms(50);

    st7565_command(CMD_SET_POWER_CONTROL | 0x7);
    tick_wait_ms(10);

    st7565_command(CMD_SET_RESISTOR_RATIO | 0x6);
 
    st7565_command(CMD_DISPLAY_ON);
    st7565_command(CMD_SET_ALLPTS_NORMAL);

    /*Set brightness*/
    st7565_command(CMD_SET_VOLUME_FIRST);
    st7565_command(CMD_SET_VOLUME_SECOND | (0x18 & 0x3f));
    
    spi_cs_dis(ST7565_DRV);   
    
    memset(lcd_fb, 0x00, sizeof(lcd_fb));
}

/**
 * Mark out a rectangle
 * @param x1 left coordinate of the rectangle
 * @param y1 top coordinate of the rectangle
 * @param x2 right coordinate of the rectangle
 * @param y2 bottom coordinate of the rectangle
 */
void st7565_set_area(int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
	last_x1 = x1;
	last_y1 = y1;
	last_x2 = x2;
	last_y2 = y2;
}

/**
 * Fill the previously marked area with a color
 * @param color fill color
 */
void st7565_fill(color_t color) 
{
     /*Return if the area is out the screen*/
    if(last_x2 < 0) return;
    if(last_y2 < 0) return;
    if(last_x1 > ST7565_HOR_RES - 1) return;
    if(last_y1 > ST7565_VER_RES - 1) return;

    /*Truncate the area to the screen*/
    int32_t act_x1 = last_x1 < 0 ? 0 : last_x1;
    int32_t act_y1 = last_y1 < 0 ? 0 : last_y1;
    int32_t act_x2 = last_x2 > ST7565_HOR_RES - 1 ? ST7565_HOR_RES - 1 : last_x2;
    int32_t act_y2 = last_y2 > ST7565_VER_RES - 1 ? ST7565_VER_RES - 1 : last_y2;
    
    int32_t x, y;
    uint8_t white = color_to1(color);

    /*Refresh frame buffer*/
    for(y= act_y1; y <= act_y2; y++) {
        for(x = act_x1; x <= act_x2; x++) {
            if (white != 0) {
                lcd_fb[x+ (y/8)*ST7565_HOR_RES] |= (1 << (7-(y%8)));
            } else {
                lcd_fb[x+ (y/8)*ST7565_VER_RES] &= ~( 1 << (7-(y%8)));
            }
        }
    }
    
    st7565_flush(act_x1, act_y1, act_x2, act_y2);
}

/**
 * Put a pixel map to the previously marked area
 * @param color_p an array of pixels
 */
void st7565_map(color_t * color_p) 
{
     /*Return if the area is out the screen*/
    if(last_x2 < 0) return;
    if(last_y2 < 0) return;
    if(last_x1 > ST7565_HOR_RES - 1) return;
    if(last_y1 > ST7565_VER_RES - 1) return;

    /*Truncate the area to the screen*/
    int32_t act_x1 = last_x1 < 0 ? 0 : last_x1;
    int32_t act_y1 = last_y1 < 0 ? 0 : last_y1;
    int32_t act_x2 = last_x2 > ST7565_HOR_RES - 1 ? ST7565_HOR_RES - 1 : last_x2;
    int32_t act_y2 = last_y2 > ST7565_VER_RES - 1 ? ST7565_VER_RES - 1 : last_y2;
    
    int32_t x, y;

    /*Set the first row in */
    
    /*Refresh frame buffer*/
    for(y= act_y1; y <= act_y2; y++) {
        for(x = act_x1; x <= act_x2; x++) {
            if (color_to1(*color_p) != 0) {
                lcd_fb[x+ (y/8)*ST7565_HOR_RES] &= ~( 1 << (7-(y%8)));
            } else {
                lcd_fb[x+ (y/8)*ST7565_HOR_RES] |= (1 << (7-(y%8)));
            }
            color_p ++;
        }
        
        color_p += last_x2 - act_x2; /*Next row*/
    }
    
    st7565_flush(act_x1, act_y1, act_x2, act_y2);
}
/**********************
 *   STATIC FUNCTIONS
 **********************/
/**
 * Flush a specific part of the buffer to the display
 * @param x1 left coordinate of the area to flush
 * @param y1 top coordinate of the area to flush
 * @param x2 right coordinate of the area to flush
 * @param y2 bottom coordinate of the area to flush
 */
static void st7565_flush(int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
    spi_cs_en(ST7565_DRV);   
    
    uint8_t c, p;
    for(p = y1 / 8; p <= y2 / 8; p++) {
        st7565_command(CMD_SET_PAGE | pagemap[p]);
        st7565_command(CMD_SET_COLUMN_LOWER | (x1 & 0xf));
        st7565_command(CMD_SET_COLUMN_UPPER | ((x1 >> 4) & 0xf));
        st7565_command(CMD_RMW);
  
         for(c = x1; c <= x2; c++) {
            st7565_data(lcd_fb[(ST7565_HOR_RES*p)+c]);
        }
    }
    
  spi_cs_dis(ST7565_DRV);   
}

/**
 * Write a command to the ST7565
 * @param cmd the command
 */
static void st7565_command(uint8_t cmd) 
{
    io_set_pin(ST7565_RS_PORT, ST7565_RS_PIN, ST7565_CMD_MODE);
    spi_xchg(ST7565_DRV, &cmd, NULL, sizeof(cmd));
}
  
/**
 * Write data to the ST7565
 * @param data the data
 */
static void st7565_data(uint8_t data) 
{
    io_set_pin(ST7565_RS_PORT, ST7565_RS_PIN, ST7565_DATA_MODE);
    spi_xchg(ST7565_DRV, &data, NULL, sizeof(data));
}

#endif