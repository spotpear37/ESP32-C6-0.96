#pragma once
#include <Arduino.h>
#include <SPI.h>
#define LCD_WIDTH  160 //LCD width
#define LCD_HEIGHT 80 //LCD height

#define SPIFreq                        80000000
#define EXAMPLE_PIN_NUM_MISO           5
#define EXAMPLE_PIN_NUM_MOSI           6
#define EXAMPLE_PIN_NUM_SCLK           7
#define EXAMPLE_PIN_NUM_LCD_CS         14
#define EXAMPLE_PIN_NUM_LCD_DC         15
#define EXAMPLE_PIN_NUM_LCD_RST        21
#define EXAMPLE_PIN_NUM_BK_LIGHT       3
#define Frequency       1000   
#define Resolution      10 

#define VERTICAL   0
#define HORIZONTAL 0

#define Offset_X 1
#define Offset_Y 26

void SPI_Init();

void LCD_Init(void);  //st7735的初始化
void LCD_SetCursor(uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t  Yend);
void LCD_addWindow(uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend,uint16_t* color);

void Backlight_Init(void);
void Set_Backlight(uint8_t Light);

// 填充整个屏幕为指定颜色 (RGB565格式)
void LCD_FillScreen(uint16_t color);

// RGB转RGB565
uint16_t RGB565(uint8_t r, uint8_t g, uint8_t b);