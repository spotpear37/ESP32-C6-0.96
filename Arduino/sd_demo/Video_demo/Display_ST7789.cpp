#include "Display_ST7789.h"
   
#define SPI_WRITE(_dat)                               SPI.transfer(_dat)
#define SPI_WRITE_Word(_dat)                          SPI.transfer16(_dat)
#define SPI_WRITE_nByte(_SetData,_ReadData,_Size)     SPI.transferBytes(_SetData,_ReadData,_Size)

void SPI_Init()
{
  SPI.begin(EXAMPLE_PIN_NUM_SCLK,EXAMPLE_PIN_NUM_MISO,EXAMPLE_PIN_NUM_MOSI); 
}

void LCD_WriteCommand(uint8_t Cmd)  
{ 
  SPI.beginTransaction(SPISettings(SPIFreq, MSBFIRST, SPI_MODE0));
  digitalWrite(EXAMPLE_PIN_NUM_LCD_CS, LOW);  
  digitalWrite(EXAMPLE_PIN_NUM_LCD_DC, LOW); 
  SPI_WRITE(Cmd);
  digitalWrite(EXAMPLE_PIN_NUM_LCD_CS, HIGH);  
  SPI.endTransaction();
}
void LCD_WriteData(uint8_t Data) 
{ 
  SPI.beginTransaction(SPISettings(SPIFreq, MSBFIRST, SPI_MODE0));
  digitalWrite(EXAMPLE_PIN_NUM_LCD_CS, LOW);  
  digitalWrite(EXAMPLE_PIN_NUM_LCD_DC, HIGH);  
  SPI_WRITE(Data);  
  digitalWrite(EXAMPLE_PIN_NUM_LCD_CS, HIGH);  
  SPI.endTransaction();
}    

void LCD_WriteData_Word(uint16_t Data)
{
  SPI.beginTransaction(SPISettings(SPIFreq, MSBFIRST, SPI_MODE0));
  digitalWrite(EXAMPLE_PIN_NUM_LCD_CS, LOW);  
  digitalWrite(EXAMPLE_PIN_NUM_LCD_DC, HIGH); 
  SPI_WRITE_Word(Data);
  digitalWrite(EXAMPLE_PIN_NUM_LCD_CS, HIGH);  
  SPI.endTransaction();
}  
void LCD_WriteData_nbyte(uint8_t* SetData,uint8_t* ReadData,uint32_t Size) 
{ 
  SPI.beginTransaction(SPISettings(SPIFreq, MSBFIRST, SPI_MODE0));
  digitalWrite(EXAMPLE_PIN_NUM_LCD_CS, LOW);  
  digitalWrite(EXAMPLE_PIN_NUM_LCD_DC, HIGH);  
  SPI_WRITE_nByte(SetData, ReadData, Size);
  digitalWrite(EXAMPLE_PIN_NUM_LCD_CS, HIGH);  
  SPI.endTransaction();
}    
 
void LCD_Reset(void)
{
  digitalWrite(EXAMPLE_PIN_NUM_LCD_CS, LOW);       
  delay(50);
  digitalWrite(EXAMPLE_PIN_NUM_LCD_RST, LOW); 
  delay(50);
  digitalWrite(EXAMPLE_PIN_NUM_LCD_RST, HIGH); 
  delay(50);
}
void LCD_Init(void)
{
  pinMode(EXAMPLE_PIN_NUM_LCD_CS, OUTPUT);
  pinMode(EXAMPLE_PIN_NUM_LCD_DC, OUTPUT);
  pinMode(EXAMPLE_PIN_NUM_LCD_RST, OUTPUT); 
  Backlight_Init();
  SPI_Init();

  LCD_Reset();
  //************* Start Initial Sequence **********// 
   LCD_WriteCommand(0x11);       //Sleep Out
  delay(120);               //DELAY120ms 
    
    
    LCD_WriteCommand(0x36);     //Memory Data Access Control
    LCD_WriteData(0xA8);    

  //-------------------------------- Frame rate setting----------------------------------// 
  LCD_WriteCommand(0xB1);     //Panel Function set
  LCD_WriteData(0x01); 
  LCD_WriteData(0x2C); 
  LCD_WriteData(0x2D); 

  LCD_WriteCommand(0xB2); 
  LCD_WriteData(0x01); 
  LCD_WriteData(0x2C); 
  LCD_WriteData(0x2D); 

  LCD_WriteCommand(0xB3); 
  LCD_WriteData(0x01); 
  LCD_WriteData(0x2C); 
  LCD_WriteData(0x2D); 
  LCD_WriteData(0x01); 
  LCD_WriteData(0x2C); 
  LCD_WriteData(0x2D); 
  
  LCD_WriteCommand(0xB4); //Column inversion 
  LCD_WriteData(0x07); 

//  TFT_SEND_CMD(0x20);     //Display Inversion Off
LCD_WriteCommand(0x21);     //Display inversion on
 
//---------------------------------ST7735S Power setting--------------------------------------// 
  LCD_WriteCommand(0xC0);     //LCM Control
  LCD_WriteData(0xA2); 
  LCD_WriteData(0x02); 
  LCD_WriteData(0x84); 
  
  LCD_WriteCommand(0xC1);   //Power Control Setting 
  LCD_WriteData(0xC5); 

  LCD_WriteCommand(0xC2);     //In Normal Mode (Full Colors) 
  LCD_WriteData(0x0A); 
  LCD_WriteData(0x00); 

  LCD_WriteCommand(0xC3);       //In Idle Mode (8-colors)
  LCD_WriteData(0x8A); 
  LCD_WriteData(0x2A); 
  
  LCD_WriteCommand(0xC4);       //In Partial Mode + Full colors 
  LCD_WriteData(0x8A); 
  LCD_WriteData(0xEE); 
  
  LCD_WriteCommand(0xC5); //VCOM Control 1 
  LCD_WriteData(0x0E); 
  
  
//--------------------------------gamma setting---------------------------------------// 

  LCD_WriteCommand(0xe0); 
  LCD_WriteData(0x0f); 
  LCD_WriteData(0x1a); 
  LCD_WriteData(0x0f); 
  LCD_WriteData(0x18); 
  LCD_WriteData(0x2f); 
  LCD_WriteData(0x28); 
  LCD_WriteData(0x20); 
  LCD_WriteData(0x22); 
  LCD_WriteData(0x1f); 
  LCD_WriteData(0x1b); 
  LCD_WriteData(0x23); 
  LCD_WriteData(0x37); 
  LCD_WriteData(0x00);  
  LCD_WriteData(0x07); 
  LCD_WriteData(0x02); 
  LCD_WriteData(0x10); 

  LCD_WriteCommand(0xe1);         //Negative Voltage Gamma Contro
  LCD_WriteData(0x0f); 
  LCD_WriteData(0x1b); 
  LCD_WriteData(0x0f); 
  LCD_WriteData(0x17); 
  LCD_WriteData(0x33); 
  LCD_WriteData(0x2c); 
  LCD_WriteData(0x29); 
  LCD_WriteData(0x2e); 
  LCD_WriteData(0x30); 
  LCD_WriteData(0x30); 
  LCD_WriteData(0x39); 
  LCD_WriteData(0x3f); 
  LCD_WriteData(0x00); 
  LCD_WriteData(0x07); 
  LCD_WriteData(0x03); 
  LCD_WriteData(0x10); 
  
  LCD_WriteCommand(0x2a);       //Column address set
  LCD_WriteData(0x00);      //start column
  LCD_WriteData(0x00+2);
  LCD_WriteData(0x00);      //end column  
  LCD_WriteData(0x80+2);

  LCD_WriteCommand(0x2b);     //Row address set  
  LCD_WriteData(0x00);      //start row
  LCD_WriteData(0x00+3);
  LCD_WriteData(0x00);      //end row
  LCD_WriteData(0x80+3);
  
  LCD_WriteCommand(0xF0); //Enable test command  
  LCD_WriteData(0x01); 
  LCD_WriteCommand(0xF6); //Disable ram power save mode 
  LCD_WriteData(0x00); 
  
  LCD_WriteCommand(0x3A); //Interface Pixel Format  
  LCD_WriteData(0x05);  //16BIT mode   

  
  LCD_WriteCommand(0x29);       //Display on
  LCD_WriteCommand(0x2C);     //Memory write

}
/******************************************************************************
function: Set the cursor position
parameter :
    Xstart:   Start uint16_t x coordinate
    Ystart:   Start uint16_t y coordinate
    Xend  :   End uint16_t coordinates
    Yend  :   End uint16_t coordinatesen
******************************************************************************/
void LCD_SetCursor(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t  y1)
{ 
  // 0x2A = Column address set (X坐标), 0x2B = Row address set (Y坐标)
  LCD_WriteCommand(0x2A);
  LCD_WriteData(x0 >> 8);
  LCD_WriteData(x0 + Offset_X);
  LCD_WriteData(x1 >> 8);
  LCD_WriteData(x1 + Offset_X);
  
  LCD_WriteCommand(0x2B);
  LCD_WriteData(y0 >> 8);
  LCD_WriteData(y0 + Offset_Y);
  LCD_WriteData(y1 >> 8);
  LCD_WriteData(y1 + Offset_Y);
  
  LCD_WriteCommand(0x2C);
}
/******************************************************************************
function: Refresh the image in an area
parameter :
    Xstart:   Start uint16_t x coordinate
    Ystart:   Start uint16_t y coordinate
    Xend  :   End uint16_t coordinates
    Yend  :   End uint16_t coordinates
    color :   Set the color
******************************************************************************/
void LCD_addWindow(uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend, uint16_t* color)
{       
  uint16_t Show_Width = Xend - Xstart + 1;
  uint16_t Show_Height = Yend - Ystart + 1;
  
  // 设置显示区域
  LCD_SetCursor(Xstart, Ystart, Xend, Yend);
  
  // 使用块传输提高效率
  SPI.beginTransaction(SPISettings(SPIFreq, MSBFIRST, SPI_MODE0));
  digitalWrite(EXAMPLE_PIN_NUM_LCD_CS, LOW);
  digitalWrite(EXAMPLE_PIN_NUM_LCD_DC, HIGH);
  
  // 一次性传输所有数据
  for (uint32_t i = 0; i < (uint32_t)Show_Width * Show_Height; i++) {
    SPI.transfer16(color[i]);
  }
  
  digitalWrite(EXAMPLE_PIN_NUM_LCD_CS, HIGH);
  SPI.endTransaction();
}
// backlight
void Backlight_Init(void)
{
  ledcAttach(EXAMPLE_PIN_NUM_BK_LIGHT, Frequency, Resolution);    
  ledcWrite(EXAMPLE_PIN_NUM_BK_LIGHT, 100);                      
}

void Set_Backlight(uint8_t Light)                        //
{

  if(Light > 100 || Light < 0)
    printf("Set Backlight parameters in the range of 0 to 100 \r\n");
  else{
    uint32_t Backlight = Light*10;
    ledcWrite(EXAMPLE_PIN_NUM_BK_LIGHT, Backlight);
  }
}

// RGB转RGB565
uint16_t RGB565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

// 填充整个屏幕为指定颜色
void LCD_FillScreen(uint16_t color) {
  // 创建一行颜色数据
  static uint16_t color_line[LCD_WIDTH];
  for (int i = 0; i < LCD_WIDTH; i++) {
    color_line[i] = color;
  }
  
  // 逐行填充
  for (int y = 0; y < LCD_HEIGHT; y++) {
    LCD_addWindow(0, y, LCD_WIDTH - 1, y, color_line);
  }
}





