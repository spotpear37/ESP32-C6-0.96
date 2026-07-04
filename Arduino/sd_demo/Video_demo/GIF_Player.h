#pragma once
#include <Arduino.h>
#include "Display_ST7789.h"
#include "loading_frames.h"

#define GIF_FRAME_WIDTH    LOADING_FRAME_WIDTH
#define GIF_FRAME_HEIGHT   LOADING_FRAME_HEIGHT
#define GIF_FRAME_COUNT    LOADING_FRAME_COUNT
#define GIF_FRAME_SIZE     LOADING_FRAME_SIZE
#define GIF_DEFAULT_FPS    10

// 播放模式
enum GIF_PlayMode {
    GIF_MODE_LOOP,      // 循环播放
    GIF_MODE_ONCE,      // 播放一次
    GIF_MODE_PINGPONG   // 往返播放
};

void GIF_Init(void);
void GIF_DrawFrame(uint8_t frameIndex);
void GIF_PlayStart(uint8_t fps, GIF_PlayMode mode);
void GIF_PlayStop(void);
void GIF_Update(void);
bool GIF_IsPlaying(void);
void GIF_SetFrame(uint8_t frameIndex);
uint8_t GIF_GetCurrentFrame(void);
