/* vim: set ai et ts=4 sw=4: */
#include "stm32f7xx_hal.h"
#include "ssd1351.h"
#include "main.h"
#include "math.h"

#define COLORSWAP ((color>>8)&0x00FF) | ((color<<8)&0xFF00)
#define DISP_WIDTH 128
#define DISP_HEIGHT 128

uint16_t videoram[DISP_HEIGHT][DISP_WIDTH]={0};


void videoram_write(uint8_t X,uint8_t Y, uint16_t color){
	if ( X>=0  			&&
		 X<DISP_WIDTH	&&
		 Y>=0	        &&
		 Y<DISP_HEIGHT ) {
		videoram[Y][X] = COLORSWAP;
	}
}

static void SSD1351_Select() {
    HAL_GPIO_WritePin(SSD1351_CS_GPIO_Port, SSD1351_CS_Pin, GPIO_PIN_RESET);
}

void SSD1351_Unselect() {
    HAL_GPIO_WritePin(SSD1351_CS_GPIO_Port, SSD1351_CS_Pin, GPIO_PIN_SET);
}

static void SSD1351_Reset() {
    HAL_GPIO_WritePin(SSD1351_RES_GPIO_Port, SSD1351_RES_Pin, GPIO_PIN_SET);
    HAL_Delay(5);
    HAL_GPIO_WritePin(SSD1351_RES_GPIO_Port, SSD1351_RES_Pin, GPIO_PIN_RESET);
    HAL_Delay(5);
    HAL_GPIO_WritePin(SSD1351_RES_GPIO_Port, SSD1351_RES_Pin, GPIO_PIN_SET);
    HAL_Delay(5);
}

/*static*/ void SSD1351_WriteCommand(uint8_t cmd) {
    HAL_GPIO_WritePin(SSD1351_DC_GPIO_Port, SSD1351_DC_Pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&SSD1351_SPI_PORT, &cmd, sizeof(cmd), HAL_MAX_DELAY);
}

static void SSD1351_WriteData(uint8_t* buff, size_t buff_size) {
    HAL_GPIO_WritePin(SSD1351_DC_GPIO_Port, SSD1351_DC_Pin, GPIO_PIN_SET);

    // split data in small chunks because HAL can't send more then 64K at once
    while(buff_size > 0) {
        uint16_t chunk_size = buff_size > 32768 ? 32768 : buff_size;
        HAL_SPI_Transmit(&SSD1351_SPI_PORT, buff, chunk_size, HAL_MAX_DELAY);
        buff += chunk_size;
        buff_size -= chunk_size;
    }
}

static void SSD1351_SetAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    // column address set
    SSD1351_WriteCommand(0x15); // SETCOLUMN
    {
        uint8_t data[] = { x0 & 0xFF, x1 & 0xFF };
        SSD1351_WriteData(data, sizeof(data));
    }

    // row address set
    SSD1351_WriteCommand(0x75); // SETROW
    {
        uint8_t data[] = { y0 & 0xFF, y1 & 0xFF };
        SSD1351_WriteData(data, sizeof(data));
    }

    // write to RAM
    SSD1351_WriteCommand(0x5C); // WRITERAM
}

void SSD1351_Init() {
    SSD1351_Select();
    SSD1351_Reset();

    // command list is based on https://github.com/adafruit/Adafruit-SSD1351-library

    SSD1351_WriteCommand(0xFD); // COMMANDLOCK
    {
        uint8_t data[] = { 0x12 };
        SSD1351_WriteData(data, sizeof(data));
    }
    SSD1351_WriteCommand(0xFD); // COMMANDLOCK
    {
        uint8_t data[] = { 0xB1 };
        SSD1351_WriteData(data, sizeof(data));
    }
//    SSD1351_WriteCommand(0xAE); // DISPLAYOFF
    SSD1351_WriteCommand(0xB3); // CLOCKDIV
    SSD1351_WriteCommand(0xF1); // 7:4 = Oscillator Frequency, 3:0 = CLK Div Ratio (A[3:0]+1 = 1..16)
    SSD1351_WriteCommand(0xCA); // MUXRATIO
    {
        uint8_t data[] = { 0x7F }; // 127
        SSD1351_WriteData(data, sizeof(data));
    }
    SSD1351_WriteCommand(0xA0); // SETREMAP
    {
        uint8_t data[] = { 0x74 };
        SSD1351_WriteData(data, sizeof(data));
    }
    SSD1351_WriteCommand(0x15); // SETCOLUMN
    {
    	uint8_t data[] = { 0x00, 0x7F };
        SSD1351_WriteData(data, sizeof(data));
    }
    SSD1351_WriteCommand(0x75); // SETROW
    {
        uint8_t data[] = { 0x00, 0x7F };
        SSD1351_WriteData(data, sizeof(data));
    }
    SSD1351_WriteCommand(0xA1); // STARTLINE
    {
        uint8_t data[] = { 0x00 }; // 96 if display height == 96
        SSD1351_WriteData(data, sizeof(data));
    }
    SSD1351_WriteCommand(0xA2); // DISPLAYOFFSET
    {
        uint8_t data[] = { 0x00 };
        SSD1351_WriteData(data, sizeof(data));
    }
    SSD1351_WriteCommand(0xB5); // SETGPIO
    {
        uint8_t data[] = { 0x00 };
        SSD1351_WriteData(data, sizeof(data));
    }
    SSD1351_WriteCommand(0xAB); // FUNCTIONSELECT
    {
        uint8_t data[] = { 0x01 };
        SSD1351_WriteData(data, sizeof(data));
    }
    SSD1351_WriteCommand(0xB1); // PRECHARGE
    {
        uint8_t data[] = { 0x32 };
        SSD1351_WriteData(data, sizeof(data));
    }
    SSD1351_WriteCommand(0xBE); // VCOMH
    {
        uint8_t data[] = { 0x05 };
        SSD1351_WriteData(data, sizeof(data));
    }
    SSD1351_WriteCommand(0xA6); // NORMALDISPLAY (don't invert)
    SSD1351_WriteCommand(0xC1); // CONTRASTABC
    {
        uint8_t data[] = { 0xC8, 0x80, 0xC8 };
        SSD1351_WriteData(data, sizeof(data));
    }
    SSD1351_WriteCommand(0xC7); // CONTRASTMASTER
    {
        uint8_t data[] = { 0x0F };
        SSD1351_WriteData(data, sizeof(data));
    }
    SSD1351_WriteCommand(0xB4); // SETVSL
    {
        uint8_t data[] = { 0xA0, 0xB5, 0x55 };
        SSD1351_WriteData(data, sizeof(data));
    }
    SSD1351_WriteCommand(0xB6); // PRECHARGE2
    {
        uint8_t data[] = { 0x01 };
        SSD1351_WriteData(data, sizeof(data));
    }
    SSD1351_WriteCommand(0xAF); // DISPLAYON

    SSD1351_Unselect();
}

void SSD1351_DrawPixel(uint16_t x, uint16_t y, uint16_t color) {
    if((x >= SSD1351_WIDTH) || (y >= SSD1351_HEIGHT))
        return;

    SSD1351_Select();

    SSD1351_SetAddressWindow(x, y, x+1, y+1);
    uint8_t data[] = { color >> 8, color & 0xFF };
    SSD1351_WriteData(data, sizeof(data));

    SSD1351_Unselect();
}

static void SSD1351_WriteChar(uint16_t x, uint16_t y, char ch, FontDef font, uint16_t color, uint16_t bgcolor) {
    uint32_t i, b, j;

    SSD1351_SetAddressWindow(x, y, x+font.width-1, y+font.height-1);

    for(i = 0; i < font.height; i++) {
        b = font.data[(ch - 32) * font.height + i];
        for(j = 0; j < font.width; j++) {
            if((b << j) & 0x8000)  {
                uint8_t data[] = { color >> 8, color & 0xFF };
                SSD1351_WriteData(data, sizeof(data));
            } else {
                uint8_t data[] = { bgcolor >> 8, bgcolor & 0xFF };
                SSD1351_WriteData(data, sizeof(data));
            }
        }
    }
}
static void SSD1351_WriteCharVR(uint16_t x, uint16_t y, char ch, FontDef font, uint16_t color, uint16_t bgcolor) {
    uint32_t i, b, j;
		int temp = color;
//    SSD1351_SetAddressWindow(x, y, x+font.width-1, y+font.height-1);

    for(i = 0; i < font.height; i++) {
        b = font.data[(ch - 32) * font.height + i];
        for(j = 0; j < font.width; j++) {
            if((b << j) & 0x8000)  {
//                uint8_t data[] = { color >> 8, color & 0xFF };
//                SSD1351_WriteData(data, sizeof(data));
								color = temp;

								if ((y+i)<128 && (x+j)<128)//проверим чтобы не выйти за области видеопамяти
								{
									videoram[y+i][x+j] = COLORSWAP;
								}

            } else {
//                uint8_t data[] = { bgcolor >> 8, bgcolor & 0xFF };
//                SSD1351_WriteData(data, sizeof(data));
								color = bgcolor;
								if ((y+i)<128 && (x+j)<128)//проверим чтобы не выйти за области видеопамяти
								{
									videoram[y+i][x+j] = COLORSWAP;
								}
            }
        }
    }
}

void SSD1351_WriteString(uint16_t x, uint16_t y, const char* str, FontDef font, uint16_t color, uint16_t bgcolor) {
    SSD1351_Select();

    while(*str) {
        if(x + font.width >= SSD1351_WIDTH) {
            x = 0;
            y += font.height;
            if(y + font.height >= SSD1351_HEIGHT) {
                break;
            }

            if(*str == ' ') {
                // skip spaces in the beginning of the new line
                str++;
                continue;
            }
        }

        SSD1351_WriteChar(x, y, *str, font, color, bgcolor);
        x += font.width;
        str++;
    }

    SSD1351_Unselect();
}
void SSD1351_WriteStringVR(uint16_t x, uint16_t y, const char* str, FontDef font, uint16_t color, uint16_t bgcolor) {
//    SSD1351_Select();

    while(*str) {
        if(x + font.width >= SSD1351_WIDTH) {
            x = 0;
            y += font.height;
            if(y + font.height >= SSD1351_HEIGHT) {
                break;
            }

            if(*str == ' ') {
                // skip spaces in the beginning of the new line
                str++;
                continue;
            }
        }

        SSD1351_WriteCharVR(x, y, *str, font, color, bgcolor);
        x += font.width;
        str++;
    }

//    SSD1351_Unselect();
}

void SSD1351_FillRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    // clipping
    if((x >= SSD1351_WIDTH) || (y >= SSD1351_HEIGHT)) return;
    if((x + w - 1) >= SSD1351_WIDTH) w = SSD1351_WIDTH - x;
    if((y + h - 1) >= SSD1351_HEIGHT) h = SSD1351_HEIGHT - y;

    SSD1351_Select();
    SSD1351_SetAddressWindow(x, y, x+w-1, y+h-1);

    uint8_t data[] = { color >> 8, color & 0xFF };
    HAL_GPIO_WritePin(SSD1351_DC_GPIO_Port, SSD1351_DC_Pin, GPIO_PIN_SET);
    for(y = h; y > 0; y--) {
        for(x = w; x > 0; x--) {
            HAL_SPI_Transmit(&SSD1351_SPI_PORT, data, sizeof(data), HAL_MAX_DELAY);
        }
    }

    SSD1351_Unselect();
}

void SSD1351_FillScreen(uint16_t color) {
    SSD1351_FillRectangle(0, 0, SSD1351_WIDTH, SSD1351_HEIGHT, color);
}

void SSD1351_DrawImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t* data) {
    if((x >= SSD1351_WIDTH) || (y >= SSD1351_HEIGHT)) return;
    if((x + w - 1) >= SSD1351_WIDTH) return;
    if((y + h - 1) >= SSD1351_HEIGHT) return;

    SSD1351_Select();
    SSD1351_SetAddressWindow(x, y, x+w-1, y+h-1);
    SSD1351_WriteData((uint8_t*)data, sizeof(uint16_t)*w*h);
    SSD1351_Unselect();
}

void SSD1351_InvertColors(bool invert) {
    SSD1351_Select();
    SSD1351_WriteCommand(invert ? 0xA7 /* INVERTDISPLAY */ : 0xA6 /* NORMALDISPLAY */);
    SSD1351_Unselect();
}

void SSD1351_DrawLine(uint8_t X,uint8_t Y,uint16_t ang,uint16_t len,uint16_t color)
{
	//������� ������
//	int ang = 260;
//	int len = 60;
//	char X = 60;
//	char Y = 60;

	//��������� ����������
	double k,y;
	double dx;
	char sector = 4;
	uint16_t i;

	if ((ang<=45) || (ang>315)) { sector = 0;}
	if ((ang>45) && (ang<=135)) { sector = 1;}
	if ((ang>135) &&(ang<=225)) { sector = 2;}
	if ((ang>225) &&(ang<=315)) { sector = 3;}

	switch (sector) {
	case 0:
		dx = cos (ang * 3.141592/180) * len;
		k =  tan (ang * 3.141592/180);
		for (i = 0; i<dx; i++)
			{
			y = k*i;
			SSD1351_DrawPixel(X + i, round(Y + y), color);
			}
		break;
	case 1:
		dx = cos ((90-ang) * 3.141592/180) * len;
		k =  tan ((90-ang) * 3.141592/180);
		for (i = 0; i<dx; i++)
			{
			y = k*i;
			SSD1351_DrawPixel( round(X + y), Y + i, color);
			}
		break;
	case 2:
		dx = cos (ang * 3.141592/180) * len *(-1);
		k =  tan (ang * 3.141592/180);
		for (i = 0; i<dx; i++)
			{
			y = k*i;
			SSD1351_DrawPixel(X - i, round(Y - y), color);
			}

		break;
	case 3:
		dx = cos ((90-ang) * 3.141592/180) * len * (-1);
		k =  tan ((90-ang) * 3.141592/180);
		for (i = 0; i<dx; i++)
			{
			y = k*i;
			SSD1351_DrawPixel(round(X - y), Y - i, color);
			}
		break;

		default:
			break;
	}
}
void SSD1351_DrawLinecoordsVR(uint8_t X,uint8_t Y, uint8_t XX,uint8_t YY, uint16_t color){
	int w;
	int h;
	int len;
	int ang;

	w = XX-X;
	h = YY-Y;
	len = sqrt(w*w + h*h);
	ang = acos((double)w/(double)len)*(double)(180/M_PI) *(h<0?(-1):1);
	SSD1351_DrawLineVR(X, Y, ang, len, color);
	SSD1351_DrawCircleVR(X, Y, 5, color);
}
void SSD1351_DrawLinecoords1VR(struct pt pt1, struct pt pt2, uint16_t color){
	int w;
	int h;
	double len;
	int ang;

	w = pt2.x - pt1.x;
	h = pt2.y - pt1.y;
	len = sqrt(w*w + h*h);
	ang = acos((double)w/len)*((double)180/M_PI) ;
	if ((w<=0)&&(h<=0)) {ang = -ang;}
	if ((w<0)&&(h>0)) {ang = ang;}
	if ((w>0)&&(h<=0)) {ang = -ang;}
	if ((w>0)&&(h>0)) {ang = ang;}

	SSD1351_DrawLineVR(pt1.x, pt1.y, ang, len, color);
}
void SSD_rect(struct pt pt1, struct pt pt2, uint16_t color){
struct pt midpoint;
struct pt ptA, ptB, ptC, ptD;
struct pt pt_a, pt_b, pt_c, pt_d;
int r;
int w;
int h;
double len_main_diag;
double angle_main_diag;
double ang = 30;

	pt_a = pt1;
	pt_d = pt2;
	pt_b.x = pt2.x;
	pt_b.y = pt1.y;
	pt_c.x = pt1.x;
	pt_c.y = pt2.y;

	midpoint.x = (pt2.x + pt1.x) / 2;
	midpoint.y = (pt2.y + pt1.y) / 2;
	w = pt2.x - pt1.x;
	h = pt2.y - pt1.y;
	r = sqrt(w*w + h*h) /2;

	len_main_diag = sqrt(w*w + h*h);
	angle_main_diag = asin((double)w/(double)len_main_diag) * (double)(180 / M_PI);

	SSD1351_DrawLineVR(pt1.x, pt1.y, 0, pt2.x-pt1.x, color);
	SSD1351_DrawLineVR(pt1.x, pt2.y, 0, pt2.x-pt1.x, color);
	SSD1351_DrawLineVR(pt1.x, pt1.y, 90, pt2.y-pt1.y, color);
	SSD1351_DrawLineVR(pt2.x, pt1.y, 90, pt2.y-pt1.y, color);
	SSD1351_DrawLineVR(midpoint.x, midpoint.y, 0, 1, color);
	angle_main_diag += 30;
	ptA.x = midpoint.x - len_main_diag * sin( (angle_main_diag*M_PI)/180);
	ptA.y = midpoint.y - len_main_diag * cos( (angle_main_diag*M_PI)/180);
	ptB.x = midpoint.x + len_main_diag * sin( (angle_main_diag*M_PI)/180);
	ptB.y = midpoint.y - len_main_diag * cos( (angle_main_diag*M_PI)/180);
	ptC.x = midpoint.x - len_main_diag * sin( (angle_main_diag*M_PI)/180);
	ptC.y = midpoint.y + len_main_diag * cos( (angle_main_diag*M_PI)/180);
	ptD.x = midpoint.x + len_main_diag * sin( (angle_main_diag*M_PI)/180);
	ptD.y = midpoint.y + len_main_diag * cos( (angle_main_diag*M_PI)/180);


//	SSD1351_DrawLinecoords1VR(ptA, ptB, SSD1351_YELLOW);
	ptA.x = (int)( (float)(pt_a.x - midpoint.x) * cos((float)ang/180*M_PI) - (float)(pt_a.y - midpoint.y)*sin((float)ang/180*M_PI) + midpoint.x );
	ptA.y = (int)( (float)(pt_a.x - midpoint.x) * sin((float)ang/180*M_PI) + (float)(pt_a.y - midpoint.y)*cos((float)ang/180*M_PI) + midpoint.y );
	ptB.x = (int)( (float)(pt_b.x - midpoint.x) * cos((float)ang/180*M_PI) - (float)(pt_b.y - midpoint.y)*sin((float)ang/180*M_PI) + midpoint.x );
	ptB.y = (int)( (float)(pt_b.x - midpoint.x) * sin((float)ang/180*M_PI) + (float)(pt_b.y - midpoint.y)*cos((float)ang/180*M_PI) + midpoint.y );
	ptC.x = (int)( (float)(pt_c.x - midpoint.x) * cos((float)ang/180*M_PI) - (float)(pt_c.y - midpoint.y)*sin((float)ang/180*M_PI) + midpoint.x );
	ptC.y = (int)( (float)(pt_c.x - midpoint.x) * sin((float)ang/180*M_PI) + (float)(pt_c.y - midpoint.y)*cos((float)ang/180*M_PI) + midpoint.y );
	ptD.x = (int)( (float)(pt_d.x - midpoint.x) * cos((float)ang/180*M_PI) - (float)(pt_d.y - midpoint.y)*sin((float)ang/180*M_PI) + midpoint.x );
	ptD.y = (int)( (float)(pt_d.x - midpoint.x) * sin((float)ang/180*M_PI) + (float)(pt_d.y - midpoint.y)*cos((float)ang/180*M_PI) + midpoint.y );

	SSD1351_DrawLinecoords1VR(ptA, ptB, SSD1351_GREEN);
	SSD1351_DrawLinecoords1VR(ptA, ptC, SSD1351_GREEN);
	SSD1351_DrawLinecoords1VR(ptB, ptD, SSD1351_GREEN);
	SSD1351_DrawLinecoords1VR(ptC, ptD, SSD1351_GREEN);






}
void SSD_rect_ang(struct pt pt1, struct pt pt2, uint16_t color, double ang){
struct pt midpoint;
struct pt ptA, ptB, ptC, ptD;
struct pt pt_a, pt_b, pt_c, pt_d;
int r;
int w;
int h;
double len_main_diag;
double angle_main_diag;
double x0, y0;

	pt_a = pt1;
	pt_d = pt2;
	pt_b.x = pt2.x;
	pt_b.y = pt1.y;
	pt_c.x = pt1.x;
	pt_c.y = pt2.y;

	midpoint.x = (pt2.x + pt1.x) / 2;
	midpoint.y = (pt2.y + pt1.y) / 2;
	x0 = midpoint.x;
	y0 = midpoint.y;
	w = pt2.x - pt1.x;
	h = pt2.y - pt1.y;
	r = sqrt(w*w + h*h) /2;

	len_main_diag = sqrt(w*w + h*h);
	angle_main_diag = asin((double)w/(double)len_main_diag) * (double)(180 / M_PI);
	ptA.x = (int)( (float)(pt_a.x - x0) * cos((float)ang*M_PI/180) - (float)(pt_a.y - y0)*sin((float)ang*M_PI/180) + x0 );
	ptA.y = (int)( (float)(pt_a.x - x0) * sin((float)ang*M_PI/180) + (float)(pt_a.y - y0)*cos((float)ang*M_PI/180) + y0 );
	ptB.x = (int)( (float)(pt_b.x - x0) * cos((float)ang*M_PI/180) - (float)(pt_b.y - y0)*sin((float)ang*M_PI/180) + x0 );
	ptB.y = (int)( (float)(pt_b.x - x0) * sin((float)ang*M_PI/180) + (float)(pt_b.y - y0)*cos((float)ang*M_PI/180) + y0 );
	ptC.x = (int)( (float)(pt_c.x - x0) * cos((float)ang*M_PI/180) - (float)(pt_c.y - y0)*sin((float)ang*M_PI/180) + x0 );
	ptC.y = (int)( (float)(pt_c.x - x0) * sin((float)ang*M_PI/180) + (float)(pt_c.y - y0)*cos((float)ang*M_PI/180) + y0 );
	ptD.x = (int)( (float)(pt_d.x - x0) * cos((float)ang*M_PI/180) - (float)(pt_d.y - y0)*sin((float)ang*M_PI/180) + x0 );
	ptD.y = (int)( (float)(pt_d.x - x0) * sin((float)ang*M_PI/180) + (float)(pt_d.y - y0)*cos((float)ang*M_PI/180) + y0 );
	SSD1351_DrawLinecoords1VR(ptA, ptB, color);
	SSD1351_DrawLinecoords1VR(ptA, ptC, color);
	SSD1351_DrawLinecoords1VR(ptB, ptD, color);
	SSD1351_DrawLinecoords1VR(ptC, ptD, color);






}

void SSD1351_DrawLineVR(uint8_t X,uint8_t Y,int16_t ang,uint16_t len,uint16_t color)
{
	//������� ������
//	int ang = 260;
//	int len = 60;
//	char X = 60;
//	char Y = 60;

	if (ang<0) {ang = 360 + ang;}
	if (ang>360) {ang = ang%360;}
	//временные переменные
	double k,y;
	double dx;
	char sector = 4;
	uint16_t i;

	if ((ang<=45) || (ang>315)) { sector = 0;}
	if ((ang>45) && (ang<=135)) { sector = 1;}
	if ((ang>135) &&(ang<=225)) { sector = 2;}
	if ((ang>225) &&(ang<=315)) { sector = 3;}

	switch (sector) {
	case 0:
		dx = cos (ang * 3.141592/180) * len;
		k =  tan (ang * 3.141592/180);
		for (i = 0; i<dx; i++)
			{
			y = k*i;
//			SSD1351_DrawPixel(X + i, round(Y + y), color);
			videoram_write(X+i, round(Y+y),color);
//			videoram[(uint8_t)(round(Y+y))][X+i] = COLORSWAP;
//			videoram[X+i][(uint8_t)(round(Y+y)+1)] = COLORSWAP;
//			videoram[X+i][(uint8_t)(round(Y+y)-1)] = COLORSWAP;

			}
		break;
	case 1:
		dx = cos ((90-ang) * 3.141592/180) * len;
		k =  tan ((90-ang) * 3.141592/180);
		for (i = 0; i<dx; i++)
			{
			y = k*i;
//			SSD1351_DrawPixel( round(X + y), Y + i, color);
			videoram_write((uint8_t)(round(X+y)), Y+i,color);
//			videoram[Y+i][(uint8_t)(round(X+y))] = COLORSWAP;
//			videoram[(uint8_t)(round(X+y)+1)][Y+i] = COLORSWAP;
//			videoram[(uint8_t)(round(X+y)-1)][Y+i] = COLORSWAP;
			}
		break;
	case 2:
		dx = cos (ang * 3.141592/180) * len *(-1);
		k =  tan (ang * 3.141592/180);
		for (i = 0; i<dx; i++)
			{
			y = k*i;
//			SSD1351_DrawPixel(X - i, round(Y - y), color);
			videoram_write(X-i, (uint8_t)(round(Y-y)),color);
//			videoram[(uint8_t)(round(Y-y))] [X-i]= COLORSWAP;
//			videoram[X-i][(uint8_t)(round(Y-y)+1)] = COLORSWAP;
//			videoram[X-i][(uint8_t)(round(Y-y)-1)] = COLORSWAP;

			}

		break;
	case 3:
		dx = cos ((90-ang) * 3.141592/180) * len * (-1);
		k =  tan ((90-ang) * 3.141592/180);
		for (i = 0; i<dx; i++)
			{
			y = k*i;
//			SSD1351_DrawPixel(round(X - y), Y - i, color);
			videoram_write((uint8_t)(round(X-y)), Y-i,color);
//			videoram[Y-i] [(uint8_t)(round(X-y))] = COLORSWAP;
//			videoram[(uint8_t)(round(X-y)+1)][Y-i] = COLORSWAP;
//			videoram[(uint8_t)(round(X-y)-1)][Y-i] = COLORSWAP;
			}
		break;

		default:
			break;
	}
}
void SSD1351_DrawCircleVR(uint16_t x0, uint16_t y0, uint8_t r, uint16_t color)
{
	int16_t f = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x = 0;
	int16_t y = r;

//	ST7789_Select();
//	ST7789_DrawPixel(x0, y0 + r, color);
	videoram_write(x0, y0 + r,color);
//	videoram[y0+r][x0] = COLORSWAP;
//	ST7789_DrawPixel(x0, y0 - r, color);
	videoram_write(x0, y0 - r,color);
//	videoram[y0-r][x0] = COLORSWAP;
//	ST7789_DrawPixel(x0 + r, y0, color);
	videoram_write(x0 + r, y0,color);
//	videoram[y0][x0+r] = COLORSWAP;
//	ST7789_DrawPixel(x0 - r, y0, color);
	videoram_write(x0 - r, y0,color);
//	videoram[y0][x0-r] = COLORSWAP;

	while (x < y) {
		if (f >= 0) {
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x;

		videoram_write(x0 + x, y0 + y, color);
//		videoram[y0+y][x0+x] = COLORSWAP;
		videoram_write(x0 - x, y0 + y, color);
//		videoram[y0+y][x0-x] = COLORSWAP;
		videoram_write(x0 + x, y0 - y, color);
//		videoram[y0-y][x0+x] = COLORSWAP;
		videoram_write(x0 - x, y0 - y, color);
//		videoram[y0-y][x0-x] = COLORSWAP;

		videoram_write(x0 + y, y0 + x, color);
//		videoram[y0+x][x0+y] = COLORSWAP;
		videoram_write(x0 - y, y0 + x, color);
//		videoram[y0+x][x0-y] = COLORSWAP;
		videoram_write(x0 + y, y0 - x, color);
//		videoram[y0-x][x0+y] = COLORSWAP;
		videoram_write(x0 - y, y0 - x, color);
//		videoram[y0-x][x0-y] = COLORSWAP;
	}
//	ST7789_UnSelect();
}

void SSD1351_clearVR(){
int i,j;
for (i = 0; i< 128; i++)
	{
	for (j = 0; j<128; j++)
		{
		videoram[i][j] = 0;
		}
	}
}

void SSD1351_FillScreenVR(uint16_t color){
	int i,j;
	for (i = 0; i< 128; i++)
		{
		for (j = 0; j<128; j++)
			{
			videoram[i][j] = COLORSWAP;
			}
		}
	SSD1351_DrawImage(0,0,128,128,(uint16_t*)videoram);
}

void SSD1351_RefreshVR(void){
	SSD1351_DrawImage(0,0,128,128,(uint16_t*)videoram);
}
void SSD1351_DrawRectangleVR(uint8_t X,uint8_t Y, uint8_t w,uint8_t h, uint16_t color){
	SSD1351_DrawLinecoordsVR(X, Y, X+w, Y, color);
	SSD1351_DrawLinecoordsVR(X, Y, X, Y+h, color);
	SSD1351_DrawLinecoordsVR(X, Y+h, X+w, Y+h, color);
	SSD1351_DrawLinecoordsVR(X+w, Y, X+w, Y+h, color);
}
