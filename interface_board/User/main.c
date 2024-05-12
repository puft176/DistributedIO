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
//#define slave_count 4 //从机的个数（从机地址默认从1开始，且是连续的）

//宏定义设置区
#define sl_ID    0x01      //从机地址
#define st_address 0x0000  //起始地址
#define sl_num   3					//读取寄存器个数
#define slave_count 3   //要读取的从机个数


/*

//按键1查看从机01的数据
//按键2查看从机02的数据
//按键3查看从机03的数据
//按键4由主机切换到从机模式（此设备作为从机地址0x02）

//主机查看某个从机的n个寄存器数据
//Host_Read03_slave(0x02,0x0000,0x0006);//参数1从机地址，参数2起始地址，参数3时寄存器个数


*/

//加入的按键切换主机模式为从机模式
int slave=0;//从机id
int host_slave_flag=0;//0-默认情况下本设备是主机，1-本设备切换为从机模式
uint8_t key_value=0;//哪一个按键按下了1-4
uint8_t key_flag=0;//key_flag等于0表示从来没有按键按下(此时一直查看从机1的数据)-----如果不添加此标志，下载程序后需要复位操作



//按键1查看从机01的数据
//按键2查看从机02的数据
//按键3查看从机03的数据
//按键4由主机切换到从机模式（此设备作为从机地址0x01）
void key_Send()
{
		
		key_value=Key_GetNum();
		switch(key_value)
		{
			case 1:
				slave=1;key_flag=1;break;//从机地址01
			case 2:
				slave=2;key_flag=1;break;//从机地址02
			case 3:
				slave=3;key_flag=1;break;//从机地址03
			case 4:
				host_slave_flag=1;key_flag=1;break;//切换为从机模式
		}
}

int main(void)
{
//	int i=sl_ID ;					//第一个从机地址
	NVIC_Configuration(); 	 //设置NVIC中断分组2:2位抢占优先级，2位响应优先级
	uart_init(9600);	 //串口1初始化为9600（只用来打印测试数据）

	LED_Init();			     //LED端口初始化
	Key_Init();          //初始化与按键连接的硬件接口
	
	
	USART1_DMA_TX_config();//DMA发送初始化
	USART1_DMA_RX_config();//DMA接收初始化
	
//	Modbus_uart2_init(4800);//初始化modbus串口2和485控制引脚	
	Modbus_TIME3_Init(7200-1,10-1);//定时器初始化参数1是重装载数，参数2是分频系数//1ms中断一次
	
	Modbus_Init();//MODBUS初始化--本机做作为从机设备地址，本机要匹配的从机地址
	USART_DMACmd(USART1,USART_DMAReq_Tx,ENABLE);//开启使能
	USART_DMACmd(USART1,USART_DMAReq_Rx,ENABLE);//开启使能
	Reg4[1] = 0x0F0F;
//	modbus.Host_End=1;//作主机时数据处理完成标志位置1，无数据正在处理
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
//		key_Send();//按键扫描
//		if(host_slave_flag==1)//这是按键4按下了，表明是从机模式（led4不停的闪烁）
//		{
//			 Modbus_Event();//Modbus事件处理函数(执行读或者写的判断)--从机地址01
//			 if(Reg[3]==0x0A)//作为从机如果寄存器的地址00 03收到了0x0A数据则打开LED3
//			 {
////				 LED1=0;
//			 }
//			 if(Reg[3]==0x0B)
//			 {
////				 LED1=1;
//			 }
////			LED4=~LED4;
//			Delay_ms(100);
//		}
//		else if(key_flag==0)//表示开机后没有按键按下（主机模式查看从机地址01的数据）
//		{
//		
//				//参数1：查看第i个从机数据
//				Host_Read03_slave(0x01,0x0000,0x0001);//参数2起始地址，参数3寄存器个数
//				if(modbus.Host_send_flag)
//				{
//					modbus.Host_Sendtime=0;//发送完毕后计数清零（距离上次的时间）
//					modbus.Host_time_flag=0;//发送数据标志位清零
//					modbus.Host_send_flag=0;//清空发送结束数据标志位
//				
//					HOST_ModbusRX();//接收数据进行处理
//				}
////				LED2=~LED2;
//				Delay_ms(1000);
//		}
//		else
//		{
//			if(modbus.Host_time_flag)//每1s发送一次数据
//			{
//				//参数1：查看第i个从机数据
//				Host_Read03_slave(slave,0x0000,0x0003);//，参数2起始地址，参数3寄存器个数
//				if(modbus.Host_send_flag)
//				{
//					modbus.Host_Sendtime=0;//发送完毕后计数清零（距离上次的时间）
//					modbus.Host_time_flag=0;//发送数据标志位清零
//					modbus.Host_send_flag=0;//清空发送结束数据标志位
//					
//					HOST_ModbusRX();//接收数据进行处理
//				}
////				LED3=~LED3;
//			}
//		}
	}
}


