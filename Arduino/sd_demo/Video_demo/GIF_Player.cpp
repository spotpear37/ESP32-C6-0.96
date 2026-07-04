#include "GIF_Player.h"

// 播放状态
static bool isPlaying = false;
static uint8_t currentFrame = 0;
static uint8_t targetFps = GIF_DEFAULT_FPS;
static GIF_PlayMode playMode = GIF_MODE_LOOP;
static uint32_t lastFrameTime = 0;
static uint32_t frameInterval = 100;  // ms per frame (100ms = 10fps)
static int8_t playDirection = 1;      // 1 = forward, -1 = backward

// 帧缓冲区 (从 Flash 读取时使用的临时缓冲区)
static uint16_t frameBuffer[GIF_FRAME_WIDTH];  // 一行像素

void GIF_Init(void) {
    isPlaying = false;
    currentFrame = 0;
    lastFrameTime = 0;
    playDirection = 1;
}

// 将 RGB565 字节数据转换为 uint16_t 数组
// 帧数据存储为低字节在前 (little-endian)
static inline uint16_t bytesToRGB565(uint8_t low, uint8_t high) {
    return (uint16_t)low | ((uint16_t)high << 8);
}

// 帧数据访问: loading_frames[frameIndex][byteOffset] (2D PROGMEM)
static inline uint8_t gifReadByte(uint8_t frameIndex, uint32_t byteOffset) {
    return loading_frames[frameIndex][byteOffset];
}

void GIF_DrawFrame(uint8_t frameIndex) {
    if (frameIndex >= GIF_FRAME_COUNT) {
        frameIndex = 0;
    }
    
    // 逐行绘制
    for (uint16_t row = 0; row < GIF_FRAME_HEIGHT; row++) {
        // 计算当前行在帧内的起始偏移
        uint32_t rowOffset = (uint32_t)row * GIF_FRAME_WIDTH * 2;
        
        // 填充一行像素数据
        for (uint16_t col = 0; col < GIF_FRAME_WIDTH; col++) {
            uint32_t pixelOffset = rowOffset + col * 2;
            uint8_t low  = gifReadByte(frameIndex, pixelOffset);
            uint8_t high = gifReadByte(frameIndex, pixelOffset + 1);
            frameBuffer[col] = bytesToRGB565(low, high);
        }
        
        // 绘制这一行
        LCD_addWindow(0, row, GIF_FRAME_WIDTH - 1, row, frameBuffer);
    }
}

void GIF_PlayStart(uint8_t fps, GIF_PlayMode mode) {
    if (fps == 0) fps = 1;
    if (fps > 60) fps = 60;
    
    targetFps = fps;
    frameInterval = 1000 / fps;
    playMode = mode;
    isPlaying = true;
    currentFrame = 0;
    playDirection = 1;
    lastFrameTime = millis();
    
    // 立即显示第一帧
    GIF_DrawFrame(currentFrame);
}

void GIF_PlayStop(void) {
    isPlaying = false;
}

void GIF_Update(void) {
    if (!isPlaying) return;
    
    uint32_t now = millis();
    if (now - lastFrameTime < frameInterval) return;
    
    lastFrameTime = now;
    
    // 更新帧索引
    switch (playMode) {
        case GIF_MODE_LOOP:
            currentFrame++;
            if (currentFrame >= GIF_FRAME_COUNT) {
                currentFrame = 0;
            }
            break;
            
        case GIF_MODE_ONCE:
            currentFrame++;
            if (currentFrame >= GIF_FRAME_COUNT) {
                currentFrame = 0;
                isPlaying = false;  // 停止播放
                return;
            }
            break;
            
        case GIF_MODE_PINGPONG:
            currentFrame += playDirection;
            if (currentFrame >= GIF_FRAME_COUNT) {
                currentFrame = GIF_FRAME_COUNT - 2;
                playDirection = -1;
            } else if (currentFrame == 0) {
                playDirection = 1;
            }
            break;
    }
    
    // 绘制当前帧
    GIF_DrawFrame(currentFrame);
}

bool GIF_IsPlaying(void) {
    return isPlaying;
}

void GIF_SetFrame(uint8_t frameIndex) {
    if (frameIndex >= GIF_FRAME_COUNT) {
        frameIndex = 0;
    }
    currentFrame = frameIndex;
    GIF_DrawFrame(currentFrame);
}

uint8_t GIF_GetCurrentFrame(void) {
    return currentFrame;
}
