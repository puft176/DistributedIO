#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "led.h"
#include "key.h"
#include "sys.h"
#include "usart.h"
#include "modbus_timer.h"
#include "rs485.h"
#include "modbus.h"
#include "dma.h"


int main(void)
{
//	int i=sl_ID ;					//第一个从机地址
	NVIC_Configuration(); 	 //设置NVIC中断分组2:2位抢占优先级，2位响应优先级
	uart_init(9600);	 //串口1初始化为9600（只用来打印测试数据）

	LED_Init();			     //LED端口初始化
	Key_Init();          //初始化与按键连接的硬件接口
	
	
	USART1_DMA_TX_config();//DMA发送初始化
	USART1_DMA_RX_config();//DMA接收初始化
	
	Modbus_TIME3_Init(7200-1,10-1);//定时器初始化参数1是重装载数，参数2是分频系数//1ms中断一次
	
	Modbus_Init();//MODBUS初始化--本机做作为从机设备地址，本机要匹配的从机地址
	USART_DMACmd(USART1,USART_DMAReq_Tx,ENABLE);//开启使能
	USART_DMACmd(USART1,USART_DMAReq_Rx,ENABLE);//开启使能
	Reg4[1] = 0x0F0F;
	while(1)
	{
		
		 Modbus_Event();//Modbus事件处理函数(执行读或者写的判断)--从机地址01
		 if(Reg4[3]==0x0A)//作为从机如果寄存器的地址00 03收到了0x0A数据则打开LED3
		 {
//				 LED1=0;
		 }
		 if(Reg4[3]==0x0B)
		 {
//				 LED1=1;
		 }
//			LED4=~LED4;
		 Delay_ms(100);
	}
}


