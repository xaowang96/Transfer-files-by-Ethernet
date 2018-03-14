#include "FreeRtOS.h"
#include "task.h"
#include "portmacro.h"
#include "TskLCD.h"
#include "lcd.h"

static const char TskLCDName[7] = "TskLCD";
static unsigned short TskLCDStackDepth = 512;						//任务堆栈深度，单位为字节(word)
static UBaseType_t TskLCDPri = 10;									//任务优先级,0为最低
static TaskHandle_t TskLCDHandler;									//任务句柄


void TskLCD(void *arg)
{
	//LCD初始化
	LCD_Init();
	POINT_COLOR = RED;
//	LCD_Display_Dir(1);								//设置屏幕显示方向为横屏
//	LCD_Scan_Dir(R2L_D2U);									//设置屏扫描方向
	LCD_Clear(BLACK);
	vTaskDelay(1000);
	while(1)
	{
		LCD_ShowString(100, 200, 12*16, 24, 24, (u8 *)"Hello World! ^-^");		//显示一个字符串
		LCD_ShowString(100, 230, 12*9, 24, 24, (u8 *)"LCD id : ");
		LCD_ShowNum(100+12*9, 230, lcddev.id, 5, 24);
		vTaskDelay(1000);													//挂起1s
	}
	
}
int TskLCDCreate(void)
{
	taskENTER_CRITICAL();           //进入临界区
	xTaskCreate(TskLCD, TskLCDName, TskLCDStackDepth, (void *)NULL, 
		TskLCDPri, (TaskHandle_t *)&TskLCDHandler);
	taskEXIT_CRITICAL();            //退出临界区
	return 0;
	
}
