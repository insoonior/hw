/**
 * @file R61581.c
 * 
 */

/*********************
 *      INCLUDES
 *********************/
#include "hw_conf.h"
#if USE_R61581 != 0

#include "R61581.h"
#include "hw/per/par.h"
#include "hw/per/io.h"
#include "hw/per/tick.h"
#include "misc/others/color.h"

/*********************
 *      DEFINES
 *********************/
#define R61581_CMD_MODE     0
#define R61581_DATA_MODE    1

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void r61581_io_init(void);
static void r61581_reset(void);
static void r61581_set_tft_spec(void);
static inline void r61581_cmd_mode(void);
static inline void r61581_data_mode(void);
static inline void r61581_cmd(uint8_t cmd);
static inline void r61581_data(uint8_t data);

/**********************
 *  STATIC VARIABLES
 **********************/
static int32_t last_x1;
static int32_t last_y1;
static int32_t last_x2;
static int32_t last_y2;
static bool cmd_mode = true;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * Initialize the R61581 display controller
 * @return HW_RES_OK or any error from hw_res_t enum
 */
void r61581_init(void)
{   
    r61581_io_init();
    
    /*Slow mode until the PLL is not started in the display controller*/
    par_set_wait_time(PAR_SLOW);
    
    r61581_reset();

    r61581_set_tft_spec();
   
    
    r61581_cmd(0x13);		//SET display on

    r61581_cmd(0x29);		//SET display on
    tick_wait_ms(30);        

    /*Parallel to max speed*/
    par_set_wait_time(0);
    
}

/**
 * Mark out a rectangle
 * @param x1 left coordinate of the rectangle
 * @param y1 top coordinate of the rectangle
 * @param x2 right coordinate of the rectangle
 * @param y2 bottom coordinate of the rectangle
 */
void r61581_set_area(int32_t x1, int32_t y1, int32_t x2, int32_t y2)
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
void r61581_fill(color_t color)
{
     /*Return if the area is out the screen*/
    if(last_x2 < 0) return;
    if(last_y2 < 0) return;
    if(last_x1 > R61581_HOR_RES - 1) return;
    if(last_y1 > R61581_VER_RES - 1) return;

    /*Truncate the area to the screen*/
    int32_t act_x1 = last_x1 < 0 ? 0 : last_x1;
    int32_t act_y1 = last_y1 < 0 ? 0 : last_y1;
    int32_t act_x2 = last_x2 > R61581_HOR_RES - 1 ? R61581_HOR_RES - 1 : last_x2;
    int32_t act_y2 = last_y2 > R61581_VER_RES - 1 ? R61581_VER_RES - 1 : last_y2;

    //Set the rectangular area
    r61581_cmd(0x002A);
    r61581_data(act_x1 >> 8);
    r61581_data(0x00FF & act_x1);
    r61581_data(act_x2 >> 8);
    r61581_data(0x00FF & act_x2);

    r61581_cmd(0x002B);
    r61581_data(act_y1 >> 8);
    r61581_data(0x00FF & act_y1);
    r61581_data(act_y2 >> 8);
    r61581_data(0x00FF & act_y2);

    r61581_cmd(0x2c);
    
    uint16_t color16 = color_to16(color);

    uint32_t size = (act_x2 - act_x1 + 1) * (act_y2 - act_y1 + 1);
    r61581_data_mode();
    par_wr_mult(color16, size);
}

/**
 * Put a pixel map to the previously marked area
 * @param color_p an array of pixels
 */
void r61581_map(color_t * color_p)
{
     /*Return if the area is out the screen*/
    if(last_x2 < 0) return;
    if(last_y2 < 0) return;
    if(last_x1 > R61581_HOR_RES - 1) return;
    if(last_y1 > R61581_VER_RES - 1) return;

    /*Truncate the area to the screen*/
    int32_t act_x1 = last_x1 < 0 ? 0 : last_x1;
    int32_t act_y1 = last_y1 < 0 ? 0 : last_y1;
    int32_t act_x2 = last_x2 > R61581_HOR_RES - 1 ? R61581_HOR_RES - 1 : last_x2;
    int32_t act_y2 = last_y2 > R61581_VER_RES - 1 ? R61581_VER_RES - 1 : last_y2;

        
    //Set the rectangular area
    r61581_cmd(0x002A);
    r61581_data(act_x1 >> 8);
    r61581_data(0x00FF & act_x1);
    r61581_data(act_x2 >> 8);
    r61581_data(0x00FF & act_x2);

    r61581_cmd(0x002B);
    r61581_data(act_y1 >> 8);
    r61581_data(0x00FF & act_y1);
    r61581_data(act_y2 >> 8);
    r61581_data(0x00FF & act_y2);

    r61581_cmd(0x2c);

    int16_t i;
    uint16_t act_w = act_x2 - act_x1 + 1;
    uint16_t last_w = last_x2 - last_x1 + 1;
    
    r61581_data_mode();
    
#if COLOR_DEPTH == 16
    for(i = act_y1; i <= act_y2; i++) {
        par_wr_array((uint16_t*)color_p, act_w);
        color_p += last_w;
    }
#else
    int16_t j;
    for(i = act_y1; i <= act_y2; i++) {
        for(j = 0; j <= act_x2 - act_x1 + 1; j++) {
            par_wr(color_to16(color_p[j]));
            color_p += last_w;
        }
    }
#endif
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/**
 * Io init 
 */
static void r61581_io_init(void)
{ 
    io_set_pin_dir(R61581_RST_PORT, R61581_RST_PIN, IO_DIR_OUT);
    io_set_pin_dir(R61581_BL_PORT, R61581_BL_PIN, IO_DIR_OUT);
    io_set_pin_dir(R61581_RS_PORT, R61581_RS_PIN, IO_DIR_OUT);

    io_set_pin(R61581_RST_PORT, R61581_RST_PIN, 1);
    io_set_pin(R61581_BL_PORT, R61581_BL_PIN, 0);
    io_set_pin(R61581_RS_PORT, R61581_RS_PIN, R61581_CMD_MODE);
    cmd_mode = true;
}

/**
 * Reset
 */
static void r61581_reset(void)
{
    /*Hardware reset*/
    io_set_pin(R61581_RST_PORT, R61581_RST_PIN, 1);
    
    tick_wait_ms(50);
    io_set_pin(R61581_RST_PORT, R61581_RST_PIN, 0);
    tick_wait_ms(50);
    io_set_pin(R61581_RST_PORT, R61581_RST_PIN, 1);
    tick_wait_ms(50);

    /*Chip enable*/
    par_cs_dis(R61581_PAR_CS);
    tick_wait_ms(10);
    par_cs_en(R61581_PAR_CS);
    tick_wait_ms(5);
    
    /*Software reset*/
    r61581_cmd(0x01);
    tick_wait_ms(20);

    r61581_cmd(0x01);
    tick_wait_ms(20);

    r61581_cmd(0x01);
    tick_wait_ms(20);
}

/**
 * TFT specific initalization
 */
static void r61581_set_tft_spec(void)
{    
    r61581_cmd(0xB0);
    r61581_data(0x00);

    r61581_cmd(0xB3);
    r61581_data(0x02);
    r61581_data(0x00);
    r61581_data(0x00);
    r61581_data(0x10); 

    r61581_cmd(0xB4);	
    r61581_data(0x00);//0X10 

    r61581_cmd(0xB9); //PWM
    r61581_data(0x01);
    r61581_data(0xFF); //FF brightness
    r61581_data(0xFF);
    r61581_data(0x18);

    r61581_cmd(0xC0);
    r61581_data(0x02);
    r61581_data(0x3B);//
    r61581_data(0x00);
    r61581_data(0x00);
    r61581_data(0x00);
    r61581_data(0x01);
    r61581_data(0x00);//NW
    r61581_data(0x43);

    r61581_cmd(0xC1);
    r61581_data(0x08);
    r61581_data(0x15);//CLOCK
    r61581_data(0x08);
    r61581_data(0x08);

    r61581_cmd(0xC4);
    r61581_data(0x15);
    r61581_data(0x03);
    r61581_data(0x03);
    r61581_data(0x01); 

    r61581_cmd(0xC6);
    r61581_data(0x02); 

    r61581_cmd(0xC8);
    r61581_data(0x0c);
    r61581_data(0x05);
    r61581_data(0x0A);
    r61581_data(0x6B);
    r61581_data(0x04);
    r61581_data(0x06);
    r61581_data(0x15);
    r61581_data(0x10);
    r61581_data(0x00);
    r61581_data(0x31); 


    r61581_cmd(0x36);
    if(R61581_ORI == 0) r61581_data(0xE0);
    else r61581_data(0x20);
    
    r61581_cmd(0x0C);
    r61581_data(0x55);

    r61581_cmd(0x3A);
    r61581_data(0x55);

    r61581_cmd(0x38); 

    r61581_cmd(0xD0);
    r61581_data(0x07);
    r61581_data(0x07);
    r61581_data(0x14);
    r61581_data(0xA2);

    r61581_cmd(0xD1);
    r61581_data(0x03);
    r61581_data(0x5A);
    r61581_data(0x10);

    r61581_cmd(0xD2);
    r61581_data(0x03);
    r61581_data(0x04);
    r61581_data(0x04);

    r61581_cmd(0x11);
    tick_wait_ms(10);

    r61581_cmd(0x2A);
    r61581_data(0x00);
    r61581_data(0x00);
    r61581_data(0x01);
    r61581_data(0xDF);//480

    r61581_cmd(0x2B);
    r61581_data(0x00);
    r61581_data(0x00);
    r61581_data(0x01);
    r61581_data(0x3F);//320

    tick_wait_ms(10);

    r61581_cmd(0x29);
    tick_wait_ms(5);

    r61581_cmd(0x2C);
    tick_wait_ms(5);
}

/**
 * Command mode
 */
static inline void r61581_cmd_mode(void)
{
    if(cmd_mode == false) {
        io_set_pin(R61581_RS_PORT, R61581_RS_PIN, R61581_CMD_MODE);
        cmd_mode = true;
    }
}

/**
 * Data mode
 */
static inline void r61581_data_mode(void)
{
    if(cmd_mode != false) {
        io_set_pin(R61581_RS_PORT, R61581_RS_PIN, R61581_DATA_MODE);
        cmd_mode = false;
    }
}

/**
 * Write command
 * @param cmd the command
 */
static inline void r61581_cmd(uint8_t cmd)
{    
    r61581_cmd_mode();
    par_wr(cmd);    
}

/**
 * Write data
 * @param data the data
 */
static inline void r61581_data(uint8_t data)
{    
    r61581_data_mode();
    par_wr(data);    
}
#endif
