#pragma once

#include <Arduino.h>

// 视频播放初始化
void Video_Init(void);

// 视频播放循环（在loop中调用）
void Video_Play_Loop(void);

// 检查是否有视频文件
bool Video_CheckFiles(void);

// 检查视频系统是否已初始化
bool Video_IsInitialized(void);

