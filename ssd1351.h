/* vim: set ai et ts=4 sw=4: */
#ifndef __SSD1351_H__
#define __SSD1351_H__

#include "fonts.h"
//#include "main.h"
#include <stdbool.h>

/*** Redefine if necessary ***/
#define SSD1351_SPI_PORT hspi2
extern SPI_HandleTypeDef SSD1351_SPI_PORT;

#define SSD1351_RES_Pin       GPIO_PIN_0
#define SSD1351_RES_GPIO_Port GPIOB
//#ifndef SSD1351_CS_Pin
//	#define SSD1351_CS_Pin        GPIO_PIN_0
//#endif
//#ifndef SSD1351_CS_GPIO_Port
//	#define SSD1351_CS_GPIO_Port  GPIOC
//#endif
#ifndef SSD1351_DC_Pin
	#define SSD1351_DC_Pin        GPIO_PIN_10
#endif
#ifndef SSD1351_DC_GPIO_Port
	#define SSD1351_DC_GPIO_Port  GPIOF
#endif

// default orientation
#define SSD1351_WIDTH  128
#define SSD1351_HEIGHT 128

/****************************/
struct pt{
	int x;
	int y;
};
struct rect {
	struct pt pt1;
	struct pt pt2;
};
/****************************/

// Color definitions
#define	SSD1351_BLACK   0x0000
#define	SSD1351_BLUE    0x001F
#define	SSD1351_RED     0xF800
#define	SSD1351_GREEN   0x07E0
#define SSD1351_CYAN    0x07FF
#define SSD1351_MAGENTA 0xF81F
#define SSD1351_YELLOW  0xFFE0
#define SSD1351_WHITE   0xFFFF
#define SSD1351_COLOR565(r, g, b) (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3))

// call before initializing any SPI devices
void SSD1351_Unselect(void);

void SSD1351_Init(void);
void SSD1351_DrawPixel(uint16_t x, uint16_t y, uint16_t color);
void SSD1351_WriteString(uint16_t x, uint16_t y, const char* str, FontDef font, uint16_t color, uint16_t bgcolor);
void SSD1351_FillRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void SSD1351_FillScreen(uint16_t color);
void SSD1351_DrawImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t* data);
void SSD1351_InvertColors(bool invert);
void SSD1351_WriteCommand(uint8_t cmd);
void SSD1351_DrawLine(uint8_t X,uint8_t Y,uint16_t ang,uint16_t len,uint16_t color);

void SSD1351_WriteStringVR(uint16_t x, uint16_t y, const char* str, FontDef font, uint16_t color, uint16_t bgcolor);
void SSD1351_FillScreenVR(uint16_t color);
void SSD1351_DrawLineVR(uint8_t X,uint8_t Y,int16_t ang,uint16_t len,uint16_t color);
void SSD1351_DrawLinecoordsVR(uint8_t X,uint8_t Y, uint8_t XX,uint8_t YY, uint16_t color);
void SSD1351_DrawLinecoords1VR(struct pt pt1, struct pt pt2, uint16_t color);
void SSD1351_DrawCircleVR(uint16_t x, uint16_t y, uint8_t r, uint16_t color);
void SSD1351_DrawRectangleVR(uint8_t X,uint8_t Y, uint8_t w,uint8_t h, uint16_t color);

void SSD1351_RefreshVR(void);
void SSD1351_clearVR(void);
void SSD_rect(struct pt pt1, struct pt pt2, uint16_t color);
void SSD_rect_ang(struct pt pt1, struct pt pt2, uint16_t color, double ang);
#endif // __SSD1351_H__
