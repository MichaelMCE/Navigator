
#include "config.h"


#if USE_SPI_ST7735
#include "SPI/ST7735/ST7735_spi.cpp"
#endif


#if USE_SPI_ST7735
#include "SPI/ST7735/ST7735_spi.cpp"
#endif


#if USE_FLEXTFT_R61408
#include "FlexIo/R61408/R61408_t41_p.cpp"
#endif


#if USE_SPI_GC9A01A2
#include "SPI/GC9A01A_rewrite/GC9A01A_spi.cpp"
//#include "SPI/GC9A01A/GC9A01A_spi.cpp"
#endif


#if USE_FLEXTFT_ILI9486
#include "FlexIo/ILI9486/ILI9486_t41_p.cpp"
#endif


#if USE_FLEXTFT_ILI9806
#include "FlexIo/ILI9806/ILI9806_t41_p.cpp"
#endif


#if USE_FLEXTFT_LG4572B
#include "FlexIo/LG4572B/LG4572B_t41_p.cpp"
#endif


#if USE_FLEXTFT_NT35510
#include "FlexIo/NT35510/NT35510_t41_p.cpp"
#endif


#if USE_FLEXTFT_NT35516
#include "FlexIo/NT35516/NT35516_t41_p.cpp"
#endif


#if USE_FLEXTFT_R61529
#include "FlexIo/R61529/R61529_t41_p.cpp"
#endif


#if USE_FLEXTFT_RM68120
#include "FlexIo/RM68120/RM68120_t41_p.cpp"
#endif


#if USE_FLEXTFT_S6D04D1
#include "FlexIo/S6D04D1/S6D04D1_t41_p.cpp"
#endif


#define COMPILEMEONCE 1
#include "tft_display.cpp"


