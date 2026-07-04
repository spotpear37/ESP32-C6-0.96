#include "SD_Card.h"
#include "Display_ST7789.h"
#include "LCD_Image.h"
#include "LVGL_Video.h"
#include "LCD_Text.h"
#include "GIF_Player.h"


// 全局变量：标记SD卡是否检测到
bool sdCardDetected = false;

void setup()
{
  // ========== 上电延时，等待电源完全稳定 ==========
  delay(1000);
  
  // 初始化串口用于调试
  Serial.begin(115200);
  delay(100);
  
  // 引脚初始化
  pinMode(EXAMPLE_PIN_NUM_LCD_CS, OUTPUT);
  digitalWrite(EXAMPLE_PIN_NUM_LCD_CS, HIGH);
  pinMode(EXAMPLE_PIN_NUM_LCD_DC, OUTPUT);
  digitalWrite(EXAMPLE_PIN_NUM_LCD_DC, HIGH);
  pinMode(EXAMPLE_PIN_NUM_LCD_RST, OUTPUT);
  digitalWrite(EXAMPLE_PIN_NUM_LCD_RST, HIGH);
  delay(100);
  
  LCD_Init();    
  Set_Backlight(100);
  
  LCD_FillScreen(RGB565(255, 0, 0));  // 红色
  delay(1000);
  
  LCD_FillScreen(RGB565(0, 255, 0));  // 绿色
  delay(1000);
  
  LCD_FillScreen(RGB565(0, 0, 255));  // 蓝色
  delay(1000);
  
  // 初始化SD卡
  SD_Init();
  
  // 初始化 GIF 播放器
  GIF_Init();
  
  // 检查SD卡是否检测到
  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    // 没有检测到SD卡，显示错误信息并播放 GIF
    sdCardDetected = false;
    LCD_ShowError("NO SD CARD");
    delay(1000);
    // 启动 GIF 循环播放 (10fps)
    GIF_PlayStart(10, GIF_MODE_LOOP);
    return;  // 没有SD卡，直接返回，不执行后续代码
  }
  
  sdCardDetected = true;
  
  // 检查是否有视频文件，如果有则初始化视频播放，否则显示图片
  if (Video_CheckFiles()) {
    // 有视频文件，初始化视频播放
    Video_Init();
  } else {
    // 没有视频文件，显示图片
    Display_Image("/",".png", 0);
    pinMode(BOOT_KEY_PIN, INPUT);   
  }

}

void loop()
{
  // 如果SD卡未检测到，播放 GIF 动画
  if (!sdCardDetected) {
    GIF_Update();
    delay(10);
    return;  // 没有SD卡，只播放GIF，不执行后续
  }
  
  // 如果视频系统已初始化，播放视频；否则显示图片
  if (Video_IsInitialized()) {
    // 播放视频
    Video_Play_Loop();
  } else {
    // 显示图片
    delay(1000);
    Image_Next_Loop("/",".png", 300);
    delay(500);
  }
}


   



