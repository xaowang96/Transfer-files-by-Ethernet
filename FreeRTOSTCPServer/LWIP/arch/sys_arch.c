/*
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
 * All rights reserved. 
 * 
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 * 
 * Author: Adam Dunkels <adam@sics.se>
 *
 */
/*  Porting by Michael Vysotsky <michaelvy@hotmail.com> August 2011   */

#define SYS_ARCH_GLOBALS

/* lwIP includes. */
#include "lwip/debug.h"
#include "lwip/def.h"
#include "lwip/lwip_sys.h"
#include "lwip/mem.h"
#include "lwip/stats.h"
#include "FreeRTOS.h"
#include "task.h"
#include "includes.h"
#include "delay.h"
#include "arch/sys_arch.h"
#include "malloc.h"

xTaskHandle xTaskGetCurrentTaskHandle( void ) PRIVILEGED_FUNCTION;

/* This is the number of threads that can be started with sys_thread_new() */
#define SYS_THREAD_MAX 6

static u16_t s_nextthread = 0;

//创建一个消息邮箱
//*mbox:消息邮箱
//size:邮箱大小
//返回值:ERR_OK,创建成功
//         其他,创建失败
err_t sys_mbox_new( sys_mbox_t *mbox, int size)
{
    (void ) size;
    //archMESG_QUEUE_LENGTH消息队列长度在sys_arch.中定义
    *mbox = xQueueCreate( archMESG_QUEUE_LENGTH, sizeof( void * ) );//创建一个消息队列
#if SYS_STATS
    ++lwip_stats.sys.mbox.used;
    if (lwip_stats.sys.mbox.max < lwip_stats.sys.mbox.used)
    {
        lwip_stats.sys.mbox.max = lwip_stats.sys.mbox.used;
    }
#endif /* SYS_STATS */
    if (*mbox == NULL) //创建失败
        return ERR_MEM;

    return ERR_OK;
}

//释放并删除一个消息邮箱
//*mbox:要删除的消息邮箱
/*
  Deallocates a mailbox. If there are messages still present in the
  mailbox when the mailbox is deallocated, it is an indication of a
  programming error in lwIP and the developer should be notified.
*/
void sys_mbox_free(sys_mbox_t * mbox)
{
    if( uxQueueMessagesWaiting( *mbox ) )
    {
        /* Line for breakpoint.  Should never break here! */
        portNOP();
#if SYS_STATS
        lwip_stats.sys.mbox.err++;
#endif /* SYS_STATS */
        // TODO notify the user of failure.
    }
    vQueueDelete( *mbox );
#if SYS_STATS
    --lwip_stats.sys.mbox.used;
#endif /* SYS_STATS */
}
//向消息邮箱中发送一条消息(必须发送成功)
//*mbox:消息邮箱
//*msg:要发送的消息
void sys_mbox_post(sys_mbox_t *mbox,void *msg)
{    
    //死循环等待消息发送成功
    while ( xQueueSendToBack(*mbox, &msg, portMAX_DELAY ) != pdTRUE ) {}
}
//尝试向一个消息邮箱发送消息
//此函数相对于sys_mbox_post函数只发送一次消息，
//发送失败后不会尝试第二次发送
//*mbox:消息邮箱
//*msg:要发送的消息
//返回值:ERR_OK,发送OK
// 	     ERR_MEM,发送失败
err_t sys_mbox_trypost(sys_mbox_t *mbox, void *msg)
{ 
    err_t result;
    if ( xQueueSend( *mbox, &msg, 0 ) == pdPASS )
    {
        result = ERR_OK;//发送成功
    }
    else
    {
        // could not post, queue must be full
        result = ERR_MEM; //发送失败
#if SYS_STATS
        lwip_stats.sys.mbox.err++;
#endif /* SYS_STATS */

    }
    return result;
}

//等待邮箱中的消息
//*mbox:消息邮箱
//*msg:消息
//timeout:超时时间，如果timeout为0的话,就一直等待
//返回值:当timeout不为0时如果成功的话就返回等待的时间，
//		失败的话就返回超时SYS_ARCH_TIMEOUT
u32_t sys_arch_mbox_fetch(sys_mbox_t *mbox, void **msg, u32_t timeout)
{ 
    void *dummyptr;
    portTickType StartTime, EndTime, Elapsed;

    StartTime = xTaskGetTickCount();

    if ( msg == NULL )//消息为空
    {
        msg = &dummyptr;//指向空指针
    }

    if ( timeout != 0 )
    {
        if ( pdTRUE == xQueueReceive( *mbox, &(*msg), timeout / portTICK_RATE_MS ) ) //在时间内获得消息
        {
            EndTime = xTaskGetTickCount();//获取节拍
            Elapsed = (EndTime - StartTime) * portTICK_RATE_MS; //算出时间

            return ( Elapsed ); //返回时间
        }
        else // 超时未接受到消息，返回最大值
        {
            *msg = NULL;

            return SYS_ARCH_TIMEOUT;
        }
    }
    else // timeout 为 0 表示永远等待消息接收成功
    {
        while( pdTRUE != xQueueReceive( *mbox, &(*msg), portMAX_DELAY ) ) {} //时间任意，直到获取到消息
        EndTime = xTaskGetTickCount();
        Elapsed = (EndTime - StartTime) * portTICK_RATE_MS;

        return ( Elapsed ); // return time blocked TODO test
    }
}
//尝试获取消息
//*mbox:消息邮箱
//*msg:消息
//返回值:等待消息所用的时间/SYS_ARCH_TIMEOUT
u32_t sys_arch_mbox_tryfetch(sys_mbox_t *mbox, void **msg)
{
	void *dummyptr;
	if ( msg == NULL )
	{
		msg = &dummyptr;
	}
	if ( pdTRUE == xQueueReceive( *mbox, &(*msg), 0 ) ) //不等待获取时间
	{
		return ERR_OK;
	}
	else
	{
		return SYS_MBOX_EMPTY;//消息为空，未获取到消息
	}
}
//检查一个消息邮箱是否有效
//*mbox:消息邮箱
//返回值:1,有效.
//      0,无效
int sys_mbox_valid(sys_mbox_t *mbox)
{  
  if (*mbox == SYS_MBOX_NULL) 
    return 0;
  else
    return 1;
} 
//设置一个消息邮箱为无效
//*mbox:消息邮箱
void sys_mbox_set_invalid(sys_mbox_t *mbox)
{
	*mbox = SYS_MBOX_NULL;
} 
//创建一个信号量
//*sem:创建的信号量
//count:信号量值
//返回值:ERR_OK,创建OK
// 	     ERR_MEM,创建失败
err_t sys_sem_new(sys_sem_t * sem, u8_t count)
{  
	vSemaphoreCreateBinary(*sem ); //创建一个信号量(0/1)
	if(*sem == NULL) //创建失败
	{
#if SYS_STATS
      ++lwip_stats.sys.sem.err;
#endif /* SYS_STATS */	
		return ERR_MEM;
	}
	if(count == 0)	// Means it can't be taken
	{
		xSemaphoreTake(*sem,1);
	}
#if SYS_STATS
	++lwip_stats.sys.sem.used;
 	if (lwip_stats.sys.sem.max < lwip_stats.sys.sem.used) {
		lwip_stats.sys.sem.max = lwip_stats.sys.sem.used;
	}
#endif /* SYS_STATS */
		
	return ERR_OK;;
} 
//等待一个信号量
//*sem:要等待的信号量
//timeout:超时时间
//返回值:当timeout不为0时如果成功的话就返回等待的时间，
//		失败的话就返回超时SYS_ARCH_TIMEOUT
u32_t sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout)
{ 
	portTickType StartTime, EndTime, Elapsed;
	StartTime = xTaskGetTickCount();//获取当前节拍数
	if(	timeout != 0)
	{
		if( xSemaphoreTake( *sem, timeout / portTICK_RATE_MS ) == pdTRUE )//成功获取到信号
		{
			EndTime = xTaskGetTickCount();
			Elapsed = (EndTime - StartTime) * portTICK_RATE_MS;
			return (Elapsed); // return time blocked TODO test	
		}
		else //超时
		{
			return SYS_ARCH_TIMEOUT;
		}
	}
	else // 直到获取成功
	{
		while( xSemaphoreTake(*sem, portMAX_DELAY) != pdTRUE){}
		EndTime = xTaskGetTickCount();
		Elapsed = (EndTime - StartTime) * portTICK_RATE_MS;

		return ( Elapsed ); // return time blocked	
		
	}
}
//发送一个信号量
//sem:信号量指针
void sys_sem_signal(sys_sem_t *sem)
{
	xSemaphoreGive(*sem);
}
//释放并删除一个信号量
//sem:信号量指针
void sys_sem_free(sys_sem_t *sem)
{
#if SYS_STATS
      --lwip_stats.sys.sem.used;
#endif /* SYS_STATS */
			
	vQueueDelete(*sem);
} 
//查询一个信号量的状态,无效或有效
//sem:信号量指针
//返回值:1,有效.
//      0,无效
int sys_sem_valid(sys_sem_t *sem)
{
  if (*sem == SYS_SEM_NULL)
    return 0;
  else
    return 1;           
} 
//设置一个信号量无效
//sem:信号量指针
void sys_sem_set_invalid(sys_sem_t *sem)
{
	*sem=SYS_SEM_NULL;
} 
//arch初始化
void sys_init(void)
{ 
    //这里,我们在该函数,不做任何事情
	// keep track of how many threads have been created
	s_nextthread = 0;
} 

/*-----------------------------------------------------------------------------------*/
                                      /*互斥锁的使用*/
/*-----------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------*/
#if LWIP_COMPAT_MUTEX == 0 //为0使用互斥信号
/* Create a new mutex*/
//创建一个信号互斥锁
err_t sys_mutex_new(sys_mutex_t *mutex) {

  *mutex = xSemaphoreCreateMutex();
		if(*mutex == NULL)
	{
#if SYS_STATS
      ++lwip_stats.sys.mutex.err;
#endif /* SYS_STATS */	
		return ERR_MEM;
	}

#if SYS_STATS
	++lwip_stats.sys.mutex.used;
 	if (lwip_stats.sys.mutex.max < lwip_stats.sys.mutex.used) {
		lwip_stats.sys.mutex.max = lwip_stats.sys.mutex.used;
	}
#endif /* SYS_STATS */
        return ERR_OK;
}
/*-----------------------------------------------------------------------------------*/
/* Deallocate a mutex*/
//删除一个互斥锁
void sys_mutex_free(sys_mutex_t *mutex)
{
#if SYS_STATS
      --lwip_stats.sys.mutex.used;
#endif /* SYS_STATS */
			
	vQueueDelete(*mutex);
}
/*-----------------------------------------------------------------------------------*/
/* Lock a mutex*/
//上锁
void sys_mutex_lock(sys_mutex_t *mutex)
{
	sys_arch_sem_wait(*mutex, 0);
}

/*-----------------------------------------------------------------------------------*/
/* Unlock a mutex*/
//解锁
void sys_mutex_unlock(sys_mutex_t *mutex)
{
	xSemaphoreGive(*mutex);
}
#endif /*LWIP_COMPAT_MUTEX*/

/*-----------------------------------------------------------------------------------*/
// TODO
//如果操作系统支持线程并且LWIP也需要使用线程那么就需要实现下面的函数
/*-----------------------------------------------------------------------------------*/

//extern OS_STK * TCPIP_THREAD_TASK_STK;//TCP IP内核任务堆栈,在lwip_comm函数定义
//创建一个新进程
//*name:进程名称
//thred:进程任务函数
//*arg:进程任务函数的参数
//stacksize:进程任务的堆栈大小
//prio:进程任务的优先级
sys_thread_t sys_thread_new(const char *name, lwip_thread_fn thread, void *arg, int stacksize, int prio)
{
	xTaskHandle CreatedTask;
	int result;

   if ( s_nextthread < SYS_THREAD_MAX )
   {
	   //vPortEnterCritical(); //进入临界区
      result = xTaskCreate( thread, ( const portCHAR * ) name, stacksize, arg, prio, &CreatedTask );
	  // vPortExitCritical(); //退出临界区
	   if(result == pdPASS)
		   return CreatedTask;
	   else 
		   return NULL;
   }
   else return NULL;
}
/*
  This optional function does a "fast" set of critical region protection to the
  value specified by pval. See the documentation for sys_arch_protect() for
  more information. This function is only required if your port is supporting
  an operating system.*/
//这些函数实现临界区保护的功能
//临界区保护
sys_prot_t sys_arch_protect(void)
{	
	//进入临界区
	return portSET_INTERRUPT_MASK_FROM_ISR();
}
//退出临界区
void sys_arch_unprotect(sys_prot_t pval)
{
	portCLEAR_INTERRUPT_MASK_FROM_ISR(pval);  //退出临界区
}
//系统断言
//暂时未用到
void sys_assert( const char *msg )
{	
	( void ) msg;
	/*FSL:only needed for debugging
	printf(msg);
	printf("\n\r");
	*/
    portSET_INTERRUPT_MASK_FROM_ISR(  );//进入临界区
    for(;;)
    ;
}
//获取系统时间
//返回值:当前系统时间(单位:毫秒)
u32_t sys_now(void)
{
//    u32_t rtos_time, lwip_time;
	return (xTaskGetTickCount());	//获取当前节拍数
//	lwip_time=(rtos_time*1000/configTICK_RATE_HZ+1);//将节拍数转换为LWIP的时间MS
//	return lwip_time; 		//返回lwip_time;
}


//随机产生DNS TXID
u16_t LWIP_RAND()
{
	static uint8_t flag=1;
	if(flag)
	{
		flag=0;
		srand(sys_now()); 
	}
	return rand();
}
////lwip延时函数
////ms:要延时的ms数
//void sys_msleep(u32_t ms)
//{
//	delay_ms(ms);
//}
////获取系统时间,LWIP1.4.1增加的函数
////返回值:当前系统时间(单位:毫秒)
//u32_t sys_now(void)
//{
//	u32_t ucos_time, lwip_time;
//	ucos_time=OSTimeGet();	//获取当前系统时间 得到的是UCSO的节拍数
//	lwip_time=(ucos_time*1000/OS_TICKS_PER_SEC+1);//将节拍数转换为LWIP的时间MS
//	return lwip_time; 		//返回lwip_time;
//}













































