#include "sys.h"	
#include "delay.h"	
#include "led.h" 
#include "key.h"
#include "usmart.h"
#include "usart.h"
#include "malloc.h"
#include "sram.h"
#include "w25qxx.h" 

#include "lwip_comm.h"
#include "FreeRTOS.h"
#include "task.h"
#include "lcd.h"
#include "TskLed.h"
#include "TskKey.h"
#include "TskLCD.h"
#include "TskInit.h"
#include "tcp_server_demo.h"

//在LCD上显示地址信息任务
//任务优先级
#define DISPLAY_TASK_PRIO	(tskIDLE_PRIORITY + 2) 
//任务堆栈大小
#define DISPLAY_STK_SIZE	128
//任务堆栈
StackType_t	DISPLAY_TASK_STK[DISPLAY_STK_SIZE];
//任务控制块
StaticTask_t DISPLAY_TASK_BUFF;
//任务名字
const char *DISPLAY_TASK_NAME = "task display";
//任务函数
void display_task(void *pdata);

//START任务
//任务优先级
#define START_TASK_PRIO		(tskIDLE_PRIORITY + 1) 
//任务堆栈大小
#define START_STK_SIZE		128
//任务堆栈
StackType_t	START_TASK_STK[DISPLAY_STK_SIZE];
//任务控制块
StaticTask_t START_TASK_BUFF;
//任务名字
const char *START_TASK_NAME = "task start";
//任务函数
void start_task(void *pdata);


void show_address(u8 mode)
{
	u8 buf[30];
	if(mode==2)
	{
		sprintf((char*)buf,"DHCP IP :%d.%d.%d.%d",lwipdev.ip[0],lwipdev.ip[1],lwipdev.ip[2],lwipdev.ip[3]);						//打印动态IP地址
		LCD_ShowString(30,260,260,24,24,buf); 
		sprintf((char*)buf,"DHCP GW :%d.%d.%d.%d",lwipdev.gateway[0],lwipdev.gateway[1],lwipdev.gateway[2],lwipdev.gateway[3]);	//打印网关地址
		LCD_ShowString(30,290,290,24,24,buf); 
		sprintf((char*)buf,"NET MASK:%d.%d.%d.%d",lwipdev.netmask[0],lwipdev.netmask[1],lwipdev.netmask[2],lwipdev.netmask[3]);	//打印子网掩码地址
		LCD_ShowString(30,320,320,24,24,buf); 
		LCD_ShowString(30,350,210,24,24,(u8 *)"Port:8088!"); 
	}
	else 
	{
		sprintf((char*)buf,"Static IP:%d.%d.%d.%d",lwipdev.ip[0],lwipdev.ip[1],lwipdev.ip[2],lwipdev.ip[3]);						//打印动态IP地址
		LCD_ShowString(30,260,260,24,24,buf); 
		sprintf((char*)buf,"Static GW:%d.%d.%d.%d",lwipdev.gateway[0],lwipdev.gateway[1],lwipdev.gateway[2],lwipdev.gateway[3]);	//打印网关地址
		LCD_ShowString(30,290,290,24,24,buf);  
		sprintf((char*)buf,"NET MASK:%d.%d.%d.%d",lwipdev.netmask[0],lwipdev.netmask[1],lwipdev.netmask[2],lwipdev.netmask[3]);	//打印子网掩码地址
		LCD_ShowString(30,320,320,24,24,buf); 
		LCD_ShowString(30,350,210,24,24,(u8 *)"Port:8088!"); 
	}	
}

int main(void)
{
	delay_init();	    	 //延时函数初始化	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);//设置中断优先级分组为组4：0位抢占优先级，0位响应优先级	
	uart_init(115200);	 	//串口初始化为115200
	LED_Init();		  	 	//初始化与LED连接的硬件接口
	LCD_Init();				//LCD初始化
	KEY_Init();	 			//初始化按键
 	usmart_dev.init(72);	//初始化USMART
	W25QXX_Init();				//初始化W25Q128
	FSMC_SRAM_Init();		//
	my_mem_init(SRAMIN);         //初始化内部内存池
	my_mem_init(SRAMEX);		//初始化外部内存池
	POINT_COLOR = RED;
	LCD_ShowString(30,30,200,24,24,(u8 *)"WARSHIP STM32F103");
	LCD_ShowString(30,60,200,24,24,(u8 *)"LWIP+FreeRTOS Test");
	LCD_ShowString(30,90,200,24,24,(u8 *)"XAOWANG");
	LCD_ShowString(30,120,200,20,24,(u8 *)"2018/1/28");
//	POINT_COLOR = BLUE; 	//蓝色字体	
	while(lwip_comm_init()) //lwip初始化
	{
		LCD_ShowString(30,200,200,20,24,(u8 *)"Lwip Init failed!"); 	//lwip初始化失败
		delay_ms(500);
		LCD_Fill(30,200,230,230,BLACK);
		delay_ms(500);
	}
	LCD_ShowString(30,200,200,20,24,(u8 *)"Lwip Init Success!"); 		//lwip初始化成功
	while(tcp_server_init() != pdPASS) 									//初始化tcp_client(创建tcp_client线程)
	{
		LCD_ShowString(30,230,200,20,24,(u8 *)"TCP Server failed!!"); //tcp客户端创建失败
		delay_ms(500);
		LCD_Fill(30,230,230,260,WHITE);
		delay_ms(500);
	}
	LCD_ShowString(30,230,230,20,24,(u8 *)"TCP Server Success!"); 			//TCP创建成功
//	
//	TskLCDCreate();
//	TskLedCreate();			//创建Led任务
	xTaskCreateStatic(start_task,
					  START_TASK_NAME,
					  (uint32_t)START_STK_SIZE,
					 (void*)NULL,
					  (UBaseType_t)START_TASK_PRIO,
					 (StackType_t *const)START_TASK_STK,
					 (StaticTask_t *const)&START_TASK_BUFF);		 //开始任务
	vTaskStartScheduler();          //开启任务调度
	return 0;
}

//start任务
void start_task(void *pdata)
{

//	pdata = pdata ;
	TskLedCreate();													//创建Led任务	
	TskKeyCreate();													//创建按键处理任务
	TskInitCreate();												//创建SD卡初始化和FATFS初始化任务
	xTaskCreateStatic(display_task,
					  DISPLAY_TASK_NAME,
					  (uint32_t)DISPLAY_STK_SIZE,
					 (void*)NULL,
					  (UBaseType_t)DISPLAY_TASK_PRIO,
					 (StackType_t *const)DISPLAY_TASK_STK,
					 (StaticTask_t *const)&DISPLAY_TASK_BUFF);		 //显示任务
	vTaskSuspend(NULL); 											//挂起start_task任务

}

//显示地址等信息
void display_task(void *pdata)
{
	while(1)
	{ 
#if LWIP_DHCP									//当开启DHCP的时候
		if(lwipdev.dhcpstatus != 0) 			//开启DHCP
		{
			show_address(lwipdev.dhcpstatus );	//显示地址信息
			vTaskSuspend(NULL);  		//显示完地址信息后挂起自身任务
		}
#else
		show_address(0); 						//显示静态地址
		vTaskSuspend(NULL);  					//显示完地址信息后挂起自身任务
#endif //LWIP_DHCP
		vTaskDelay(100);
	}
}