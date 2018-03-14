#include "FreeRtOS.h"
#include "task.h"
#include "portmacro.h"
#include "key.h"
#include "lcd.h"
#include "stmflash.h"
#include "TskKey.h"
static const char TskKeyName[7] = "TskKey";
static unsigned short TskKeyStackDepth = 128;						//任务堆栈深度，单位为字节(word)
static UBaseType_t TskKeyPri = 5;									//任务优先级,0为最低
static TaskHandle_t TskKeyHandler;									//任务句柄
	
	
//要写入到STM32 FLASH的字符串数组
const u8 TEXT_Buffer[]={"STM32F103 FLASH TEST"};
#define SIZE sizeof(TEXT_Buffer)		//数组长度
#define FLASH_SAVE_ADDR  0X08014000		//设置FLASH 保存地址80K(必须为偶数，且其值要大于本代码所占用FLASH的大小+0X08000000)

//任务函数
void TskKey(void *arg)
{
	u8 key;
	u8 datatemp[SIZE];
	while(1)
	{
		key=KEY_Scan(0);
		if(key==KEY1_PRES)	//KEY1按下,写入STM32 FLASH
		{
//			LCD_Fill(30,400,300,500,BLACK);//清除半屏 	
			LCD_ShowStringColor(30,430,240,24,24,datatemp, BLACK);		//擦除读到的字符串
			
 			LCD_ShowString(30,400,360,24,24,(u8 *)"Start Write FLASH....");
			STMFLASH_Write(FLASH_SAVE_ADDR,(u16*)TEXT_Buffer,SIZE);
			LCD_ShowString(30,400,270,24,24,(u8 *)"FLASH Write Finished!");//提示传送完成
		}
		if(key==KEY0_PRES)	//KEY0按下,读取字符串并显示
		{		
 			LCD_ShowString(30,400,240,24,24,(u8 *)"Start Read FLASH.... ");
			STMFLASH_Read(FLASH_SAVE_ADDR,(u16*)datatemp,SIZE);
			LCD_ShowString(30,400,270,24,24,(u8 *)"The Data Readed Is:  ");//提示传送完成
			LCD_ShowString(30,430,240,24,24,datatemp);//显示读到的字符串
		}
		vTaskDelay(10);		   
	} 
}

int TskKeyCreate(void)
{
	BaseType_t err = xTaskCreate(TskKey, TskKeyName, TskKeyStackDepth, (void *)NULL, 
		TskKeyPri, (TaskHandle_t *)&TskKeyHandler);
	
	return 0;
}
