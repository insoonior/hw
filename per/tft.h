/**
 * @file tft.h
 *
 */

#ifndef TFT_H
#define TFT_H

/*********************
 *      INCLUDES
 *********************/
#include "hw_conf.h"

#if USE_TFT != 0
#include "hw/hw.h"
#include "psp/psp_tft.h"
#include "misc/others/color.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
hw_res_t tft_init(void);
void tft_set_area(int32_t x1, int32_t y1, int32_t x2, int32_t y2);
void tft_fill(color_t color);
void tft_map(color_t * color_p);

/**********************
 *      MACROS
 **********************/

#endif

#endif
