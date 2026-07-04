/***
 * SD卡视频播放功能
 * 参考main文件夹实现，使用LVGL_Image的显示屏驱动和引脚配置
 * 
 * Required libraries:
 * JPEGDEC: https://github.com/bitbank2/JPEGDEC.git
 */

#include "LVGL_Video.h"
#include "Display_ST7789.h"
#include "SD_Card.h"
#include "LCD_Image.h"  // 包含BOOT_KEY_PIN定义
#include "MjpegClass.h"
#include <SD.h>
#include <FS.h>

// 视频文件列表
const char* videoFiles[] = {
  "/video1.mjpeg", 
  "/video2.mjpeg", 
  "/video3.mjpeg"
};
const int TOTAL_VIDEOS = sizeof(videoFiles) / sizeof(videoFiles[0]);

#define FPS 20 // 目标帧率
#define PRELOAD_THRESHOLD 0.8f // 预加载阈值
// 减小MJPEG缓冲区大小，不需要整个屏幕大小，只需要足够存储一个JPEG帧
// 使用较小的缓冲区，ESP32-C6内存有限
#define MJPEG_BUFFER_SIZE (100 * 1024) // 48KB缓冲区，足够存储一个JPEG帧

// 全局变量
static uint8_t currentVideo = 0;  // 当前视频索引
static uint8_t nextVideoIndex = 0; // 下一个视频索引
static bool isFirstPlay = true;   // 标记是否是第一次播放
static bool manualSwitchRequested = false; // 手动切换请求标志

// 按键相关变量
static volatile bool buttonPressed = false; // 按键按下标志
const unsigned long buttonDebounceTime = 300; // 按键消抖时间(ms)

// 动态分配缓冲区，避免静态分配过大导致堆栈溢出
static uint8_t* mjpeg_buf = nullptr; 

// 文件对象
static File vFile; // 当前视频文件
static File nextVFile; // 下一个视频文件（预加载）

static MjpegClass mjpeg;

// 视频系统初始化标志
static bool video_initialized = false; // 视频系统是否已初始化

// MCU块缓冲区 - 用于存储单个MCU块的数据，然后一次性显示
static uint16_t* mcu_buffer = nullptr;
static int mcu_buffer_size = 0;
static bool frame_started = false; // 标记新帧开始
static int last_frame_y = -1; // 记录上一帧的y坐标

// 绘制回调函数 - 直接显示MCU块，减少SPI传输次数
static int drawMCU(JPEGDRAW *pDraw) {
  // 分配MCU块缓冲区（只分配一次，足够存储最大的MCU块）
  if (!mcu_buffer) {
    // MCU块最大通常是16x16，分配稍大一些以应对特殊情况
    mcu_buffer_size = 32 * 32; // 最大32x32的MCU块
    mcu_buffer = (uint16_t*)malloc(mcu_buffer_size * sizeof(uint16_t));
    if (!mcu_buffer) {
      Serial.println("Failed to allocate MCU buffer!");
      return 0;
    }
  }
  
  // 获取MCU块的参数
  uint16_t* pixels = (uint16_t*)pDraw->pPixels;
  int x = pDraw->x;
  int y = pDraw->y;
  int width = pDraw->iWidth;
  int height = pDraw->iHeight;
  
  // 检测新帧开始（y坐标从0或更小的值开始，或者y坐标回退）
  // 注意：必须在坐标调整前检测，使用原始y坐标
  int original_y = y;
  bool is_new_frame = false;
  
  if (last_frame_y == -1) {
    // 第一帧
    is_new_frame = true;
  } else if (original_y < last_frame_y) {
    // y坐标回退，说明是新帧开始
    is_new_frame = true;
  }
  
  if (is_new_frame) {
    frame_started = true;

    if (original_y > 0 && original_y < LCD_HEIGHT) {
      // 使用静态缓冲区，避免重复分配
      static uint16_t black_line[LCD_WIDTH];
      static bool black_line_initialized = false;
      
      if (!black_line_initialized) {
        for (int i = 0; i < LCD_WIDTH; i++) {
          black_line[i] = 0x0000;
        }
        black_line_initialized = true;
      }
    
      const int CLEAR_BLOCK = 32;  // 每次清空32行，减少传输次数
      for (int clear_y = 0; clear_y < original_y; clear_y += CLEAR_BLOCK) {
        int block_h = (clear_y + CLEAR_BLOCK > original_y) ? (original_y - clear_y) : CLEAR_BLOCK;
        // 确保不越界
        if (clear_y + block_h > LCD_HEIGHT) {
          block_h = LCD_HEIGHT - clear_y;
        }
        if (block_h > 0 && clear_y < LCD_HEIGHT) {
          // 使用块清空，一次性清空多行
          for (int i = 0; i < block_h; i++) {
            if (clear_y + i < LCD_HEIGHT) {
              LCD_addWindow(0, clear_y + i, LCD_WIDTH - 1, clear_y + i, black_line);
            }
          }
        }
      }
    }
  }
  
  if (original_y >= 0 && original_y < LCD_HEIGHT) {
    last_frame_y = original_y;
  }
  
  if (x < 0) {
    width += x;  // 如果x<0，调整width
    x = 0;
  }
  if (y < 0) {
    height += y;  // 如果y<0，调整height
    y = 0;
  }
  if (x + width > LCD_WIDTH) {
    width = LCD_WIDTH - x;
  }
  if (y + height > LCD_HEIGHT) {
    height = LCD_HEIGHT - y;
  }
  
  // 检查调整后的尺寸是否有效
  if (width <= 0 || height <= 0 || x >= LCD_WIDTH || y >= LCD_HEIGHT) {
    return 1;  // 跳过无效的MCU块
  }
  
  // 检查缓冲区大小是否足够
  int needed_size = width * height;
  if (needed_size > mcu_buffer_size) {
    // 重新分配更大的缓冲区
    free(mcu_buffer);
    mcu_buffer_size = needed_size;
    mcu_buffer = (uint16_t*)malloc(mcu_buffer_size * sizeof(uint16_t));
    if (!mcu_buffer) {
      Serial.println("Failed to reallocate MCU buffer!");
      return 0;
    }
  }
  

  // JPEGDEC 输出 RGB565 大端格式，需要转换为小端以匹配 LCD
  for (int j = 0; j < height; j++) {
    for (int i = 0; i < width; i++) {
      uint16_t pixel = pixels[j * pDraw->iWidth + i];
      // 转换字节序：大端 -> 小端
      mcu_buffer[j * width + i] = ((pixel >> 8) & 0xFF) | ((pixel << 8) & 0xFF00);
    }
  }
  

  uint16_t x_end = x + width - 1;
  uint16_t y_end = y + height - 1;
  
  if (x_end >= LCD_WIDTH) x_end = LCD_WIDTH - 1;
  if (y_end >= LCD_HEIGHT) y_end = LCD_HEIGHT - 1;
  
  LCD_addWindow(x, y, x_end, y_end, mcu_buffer);
  
  return 1;
}

// 显示完整的帧（不再需要，因为drawMCU直接显示）
static void displayFrame() {
  // drawMCU已经直接显示，这里不需要额外操作
}

// 内存监控
static void printMemoryStats() {
  Serial.printf("Heap: Free=%d, Min=%d, MaxAlloc=%d\n", 
                ESP.getFreeHeap(), 
                ESP.getMinFreeHeap(),
                ESP.getMaxAllocHeap());
}

// 释放当前视频资源
static void releaseCurrentVideo() {
  if (vFile) {
    vFile.close();
    vFile = File(); // 重置文件对象
  }
}

// 释放下一个视频资源
static void releaseNextVideo() {
  if (nextVFile) {
    nextVFile.close();
    nextVFile = File();
  }
}

// BOOT按钮中断服务程序
void IRAM_ATTR buttonISR() {
  // 简单的消抖处理
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  
  if (interrupt_time - last_interrupt_time > buttonDebounceTime) {
    buttonPressed = true;
  }
  last_interrupt_time = interrupt_time;
}

// 预加载下一个视频
static bool preloadNextVideo() {
  // 确定下一个视频索引 (0-2循环)
  nextVideoIndex = (currentVideo + 1) % TOTAL_VIDEOS;
  
  // 释放之前的预加载
  releaseNextVideo();
  
  const char *nextFilename = videoFiles[nextVideoIndex];
  
  // 尝试打开下一个视频文件
  nextVFile = SD.open(nextFilename);
  if (!nextVFile || nextVFile.isDirectory()) {
    Serial.printf("Preload failed for: %s\n", nextFilename);
    return false;
  }
  
  Serial.printf("Preloaded next video: %s\n", nextFilename);
  return true;
}

// 切换到下一个视频
static void switchToNextVideo() {
  // 释放当前视频资源
  releaseCurrentVideo();
  
  // 切换到预加载的视频
  currentVideo = nextVideoIndex;
  vFile = nextVFile;
  nextVFile = File(); // 重置预加载文件
  
  // 预加载下一个视频
  preloadNextVideo();
  
  // 重置手动切换标志
  manualSwitchRequested = false;
}

// 播放视频函数
static void playVideo() {
  const char *filename = videoFiles[currentVideo];
  
  // 内存检查
  printMemoryStats();
  
  // 尝试打开文件
  if (!vFile) {
    vFile = SD.open(filename);
  }
  
  if (!vFile || vFile.isDirectory()) {
    Serial.printf("FATAL: Failed to open %s\n", filename);
    
    // 切换到下一个视频
    currentVideo = (currentVideo + 1) % TOTAL_VIDEOS;
    return;
  }

  Serial.printf("Playing: %s (Size: %d bytes)\n", filename, vFile.size());
  
  // 初始化显示 - 仅第一次播放时执行
  if (isFirstPlay) {
    // 清屏 - 创建黑色缓冲区
    static uint16_t black_buffer[LCD_WIDTH];
    for (int i = 0; i < LCD_WIDTH; i++) {
      black_buffer[i] = 0x0000;
    }
    for (int y = 0; y < LCD_HEIGHT; y++) {
      LCD_addWindow(0, y, LCD_WIDTH - 1, y, black_buffer);
    }
    isFirstPlay = false;
  }
  
  // 检查缓冲区是否已分配
  if (!mjpeg_buf) {
    Serial.println("MJPEG buffer not allocated!");
    releaseCurrentVideo();
    currentVideo = (currentVideo + 1) % TOTAL_VIDEOS;
    return;
  }
  
  // 使用动态分配的缓冲区
  // useBigEndian=true: JPEGDEC 输出大端 RGB565，drawMCU 中转换字节序
  if (!mjpeg.setup(&vFile, mjpeg_buf, drawMCU, true, 0, 0, LCD_WIDTH, LCD_HEIGHT)) {
    Serial.println("MJPEG setup failed");
    releaseCurrentVideo();
    
    // 切换到下一个视频
    currentVideo = (currentVideo + 1) % TOTAL_VIDEOS;
    return;
  }

  // 初始化计时器
  unsigned long start_ms = millis();
  unsigned long next_frame_ms = start_ms;
  
  // 获取文件总大小用于进度计算
  size_t totalSize = vFile.size();
  size_t currentPosition = 0;
  
  while (vFile.available() && mjpeg.readMjpegBuf()) {
    // 获取当前文件位置用于预加载判断
    currentPosition = vFile.position();
    
    // 在播放进度达到阈值时预加载下一个视频
    if (currentPosition > 0 && totalSize > 0 && 
        static_cast<float>(currentPosition) / totalSize > PRELOAD_THRESHOLD) {
      if (!nextVFile) {
        preloadNextVideo();
      }
    }
    
    // 帧率控制逻辑 - 严格按照main文件夹的逻辑
    unsigned long curr_ms = millis();

    if (curr_ms >= next_frame_ms) {

      last_frame_y = -1;
      frame_started = false;
    
      if (!mjpeg.drawJpg()) {
        Serial.println("Draw JPG failed");
        break;
      }
      
      next_frame_ms = curr_ms + (1000 / FPS);
      
    } else {
      // 等待下一帧时间
      delay(1);
    }
    
    // 检查按钮按下事件
    if (buttonPressed) {
      buttonPressed = false;
      manualSwitchRequested = true;
      Serial.println("Button pressed! Requesting manual switch");
      break;
    }
  }

  Serial.println("Playback ended");
  
  // 检查是否需要切换视频 - 仅在明确请求时切换（参考main文件夹逻辑）
  if (manualSwitchRequested) {

    if (nextVFile) {
      switchToNextVideo();
    } else {
      currentVideo = (currentVideo + 1) % TOTAL_VIDEOS;
    }
  }
  // 非手动切换时，currentVideo保持不变，下次继续播放同一个视频
  
  // 释放当前视频资源（无论是否切换，都需要释放，因为下次播放需要重新打开）
  releaseCurrentVideo();
}

// 检查视频文件是否存在
bool Video_CheckFiles(void) {
  bool foundAny = false;
  
  for (int i = 0; i < TOTAL_VIDEOS; i++) {
    if (SD.exists(videoFiles[i])) {
      foundAny = true;
      File testFile = SD.open(videoFiles[i]);
      if (testFile) {
        Serial.printf("%s found, Size: %d bytes\n", videoFiles[i], testFile.size());
        testFile.close();
      }
    } else {
      Serial.printf("%s not found!\n", videoFiles[i]);
    }
  }
  
  return foundAny;
}

// 视频播放初始化
void Video_Init(void) {
  Serial.println("Video system initializing...");
  
  // 打印内存初始状态
  printMemoryStats();

  // 初始化BOOT按钮
  pinMode(BOOT_KEY_PIN, INPUT_PULLUP);
  // 使用CHANGE模式，可以检测按下和释放
  attachInterrupt(digitalPinToInterrupt(BOOT_KEY_PIN), buttonISR, FALLING);
  Serial.printf("Button initialized on pin %d (INPUT_PULLUP, FALLING interrupt)\n", BOOT_KEY_PIN);
  Serial.printf("Initial button state: %d (0=low/pressed, 1=high/released)\n", digitalRead(BOOT_KEY_PIN));
  
  // 分配MJPEG缓冲区（动态分配，避免静态分配过大）
  mjpeg_buf = (uint8_t*)malloc(MJPEG_BUFFER_SIZE);
  if (!mjpeg_buf) {
    Serial.println("Failed to allocate MJPEG buffer!");
    Serial.printf("Available heap: %d bytes\n", ESP.getFreeHeap());
    return;
  }
  Serial.printf("MJPEG buffer allocated: %d bytes\n", MJPEG_BUFFER_SIZE);
  
  // 检查所有视频文件
  Serial.println("Checking video files...");
  if (!Video_CheckFiles()) {
    Serial.println("No video files found!");
    return;
  }

  // 直接打开第一个视频文件（索引0）
  vFile = SD.open(videoFiles[currentVideo]);
  if (!vFile || vFile.isDirectory()) {
    Serial.printf("Failed to open first video: %s\n", videoFiles[currentVideo]);
    // 尝试打开下一个视频
    currentVideo = (currentVideo + 1) % TOTAL_VIDEOS;
    vFile = SD.open(videoFiles[currentVideo]);
    if (!vFile || vFile.isDirectory()) {
      Serial.println("Failed to open any video.");
      return;
    }
  }
  Serial.printf("First video opened: %s\n", videoFiles[currentVideo]);
  
  // 预加载下一个视频
  preloadNextVideo();
  
  video_initialized = true;
  Serial.println("Video system ready");
  printMemoryStats();
}

// 检查视频系统是否已初始化
bool Video_IsInitialized(void) {
  return video_initialized;
}

// 视频播放循环（在loop中调用）
void Video_Play_Loop(void) {
  // 检查视频系统是否已初始化
  if (!video_initialized) {
    return;
  }
  
  // 确保文件已打开
  if (!vFile) {
    const char *filename = videoFiles[currentVideo];
    vFile = SD.open(filename);
    if (!vFile || vFile.isDirectory()) {
      Serial.printf("Failed to open video: %s\n", filename);
      // 尝试下一个视频
      currentVideo = (currentVideo + 1) % TOTAL_VIDEOS;
      return;
    }
  }
  
  // 播放当前视频
  playVideo();
  
  // 短暂延迟，避免CPU过载
  delay(10);
}

