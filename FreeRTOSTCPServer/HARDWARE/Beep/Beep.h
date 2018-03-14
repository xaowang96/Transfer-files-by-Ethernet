#ifndef _BEEP_H
#define _BEEP_H

#include "sys.h"
//蜂鸣器端口定义

#define BEEP PBout(8)	// BEEP,蜂鸣器接口
void BeepInit(void );	//初始化
#endif
