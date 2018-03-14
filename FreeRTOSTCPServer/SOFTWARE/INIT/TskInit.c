#include "FreeRtOS.h"
#include "task.h"
#include "portmacro.h"
#include "led.h"
#include "lcd.h"
#include "sdio_sdcard.h"  
#include "w25qxx.h"    
#include "ff.h"  
#include "exfuns.h"
#include "TskInit.h"
static const char TskInitName[7] = "TskInit";
static unsigned short TskInitStackDepth = 128;						//任务堆栈深度，单位为字节(word)
static UBaseType_t TskInitPri = 5;									//任务优先级,0为最低
static TaskHandle_t TskInitHandler;									//任务句柄

//任务函数
void TskInit(void *arg)
{
	u32 total,free;
	u8 res;
	while(SD_Init())//检测不到SD卡
	{
		LCD_ShowString(30,400,200,24,24,(u8 *)"SD Card Error!");
		vTaskDelay(100);					
		LCD_ShowString(30,400,200,24,24,(u8 *)"Please Check! ");
		vTaskDelay(100);
	}
	exfuns_init();							//为fatfs相关变量申请内存				 
  	f_mount(fs[0],"0:",1); 					//挂载SD卡 
 	res=f_mount(fs[1],"1:",1); 				//挂载FLASH.	
	if(res==0X0D)//FLASH磁盘,FAT文件系统错误,重新格式化FLASH
	{
		LCD_ShowString(30,400,200,24,24,(u8 *)"Flash Disk Formatting...");	//格式化FLASH
		res=f_mkfs("1:",1,4096);//格式化FLASH,1,盘符;1,不需要引导区,8个扇区为1个簇
		if(res==0)
		{
			f_setlabel((const TCHAR *)"1:ALIENTEK");	//设置Flash磁盘的名字为：ALIENTEK
			LCD_ShowString(30,400,200,24,24,(u8 *)"Flash Disk Format Finish");	//格式化完成
		}
		else 
			LCD_ShowString(30,400,200,24,24,(u8 *)"Flash Disk Format Error ");	//格式化失败
		vTaskDelay(1000);
	}													    
	LCD_Fill(30,400,240,400+24,BLACK);		//清除显示			  
	while(exf_getfree((u8 *)"1:",&total,&free))	//得到外部FLASH的总容量和剩余容量
	{
		LCD_ShowString(30,400,200,24,24,(u8 *)"FLASH Fatfs Error!");
		vTaskDelay(200);
		LCD_Fill(30,400,240,400+24,BLACK);	//清除显示			  
		vTaskDelay(200);
	}
	LCD_ShowString(30,400,200,24,24,(u8 *)"FATFS OK!");	 
	LCD_ShowString(30,430,300,24,24,(u8 *)"FLASH Total Size :    MB");	 
	LCD_ShowString(30,460,300,24,24,(u8 *)"FLASH  Free Size :    MB"); 	    
 	LCD_ShowNum(30+12*19,430,total>>10,2,24);					//显示FLASH卡总容量 MB
 	LCD_ShowNum(30+12*19,460,free>>10,2,24);					//显示FLASH卡剩余容量 MB
	while(exf_getfree((u8 *)"0",&total,&free))	//得到SD卡的总容量和剩余容量
	{
		LCD_ShowString(30,400,240,24,24,(u8 *)"SD Card Fatfs Error!");
		vTaskDelay(200);
		LCD_Fill(30,400,240,400+24,BLACK);	//清除显示			  
		vTaskDelay(200);
	}
	LCD_ShowString(30,490,300,24,24,(u8 *)"SD Total Size :     MB");	 
	LCD_ShowString(30,520,300,24,24,(u8 *)"SD  Free Size :     MB"); 	    
	LCD_ShowNum(30+12*16,490,total>>10,3,24);					//显示SD卡总容量 MB
	LCD_ShowNum(30+12*16,520,free>>10,3,24);					//显示SD卡剩余容量 MB
	vTaskSuspend(NULL);  					//挂起自身任务
}
int TskInitCreate(void)
{
	BaseType_t err = xTaskCreate(TskInit, TskInitName, TskInitStackDepth, (void *)NULL, 
		TskInitPri, (TaskHandle_t *)&TskInitHandler);
	
	return 0;
}
