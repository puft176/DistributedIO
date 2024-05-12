#include "dma.h"
#include "stm32f10x_dma.h"
#include "modbus.h"
#include "usart.h"
#include "string.h"
#include "delay.h"
#include "led.h"
//u8 SendBuffe[SendBuff_Size];//定义一个发送数组

//typedef struct
//{
//  uint32_t DMA_PeripheralBaseAddr;   // 外设地址
//  uint32_t DMA_MemoryBaseAddr;       // 存储器地址
//  uint32_t DMA_DIR;                  // 传输方向
//  uint32_t DMA_BufferSize;           // 传输数目
//  uint32_t DMA_PeripheralInc;        // 外设地址增量模式
//  uint32_t DMA_MemoryInc;            // 存储器地址增量模式
//  uint32_t DMA_PeripheralDataSize;   // 外设数据宽度
//  uint32_t DMA_MemoryDataSize;       // 存储器数据宽度
//  uint32_t DMA_Mode;                 // 模式选择
//  uint32_t DMA_Priority;             // 通道优先级
//  uint32_t DMA_M2M;                  // 存储器到存储器模式
//}DMA_InitTypeDef;
					
//Memory->p(USART->DR)

void USART1_DMA_TX_config(void)
{
	//1-要初始化结构体肯定要定义一个结构体变量
	DMA_InitTypeDef DMA_InitStruct;
	//2、配置DMA时钟（通过查看手册可以知道USART1_TX使用DMA1的通道4）
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);//使能DMA1通道4的时钟
	
	//3、数据从哪里来到哪里去（配置3个）
	//这是外设地址（串口数据寄存器地址)
	DMA_InitStruct.DMA_PeripheralBaseAddr=(u32)&USART1->DR;//外设地址（或者USART1_BASE+0x04）数据寄存器(USART_DR)
	//存储器地址
	DMA_InitStruct.DMA_MemoryBaseAddr=(uint32_t)&modbus.sendbuf;//存储器地址（指向了发送数组的首地址）
	//方向：存储器到外设（外设作为目的地）
	DMA_InitStruct.DMA_DIR=DMA_DIR_PeripheralDST;//传输方式--外设作为目的地
	//规定外设是作为数据传输的目的地还是来源（数据传输方向）
	//DMA_DIR_PeripheralDST	外设作为数据传输的目的地
	//DMA_DIR_PeripheralSRC	外设作为数据传输的来源
	
	
	//4、传多少，单位是多少
	DMA_InitStruct.DMA_BufferSize= read_num*2+5;//传输数目（数组的长度）：read_num*2+5
	
	//只有一个串口数据寄存器不需要递增，数组是U8类型,所以一次传输一个字节
	DMA_InitStruct.DMA_PeripheralInc=DMA_PeripheralInc_Disable ;//外设地址b不需要递增
	DMA_InitStruct.DMA_PeripheralDataSize= DMA_PeripheralDataSize_Byte  ;//数据宽度，u8类型，一个字节
	
	//配置memory（定义了一个数组，发送一个会继续下一个，所以地址是递增）
	DMA_InitStruct.DMA_MemoryInc=DMA_MemoryInc_Enable ;//内存地址递增
	DMA_InitStruct.DMA_MemoryDataSize=DMA_MemoryDataSize_Byte  ;//一个字节
	
	
	//5、配置模式和优先级
	DMA_InitStruct.DMA_Mode=DMA_Mode_Normal  ;//循环模式DMA_Mode_Normal,DMA_Mode_Circular
	DMA_InitStruct.DMA_Priority=DMA_Priority_High ;//共有四种优先级
	DMA_InitStruct.DMA_M2M=DMA_M2M_Disable  ;//不使用M-M模式
	DMA_Init(DMA1_Channel4, &DMA_InitStruct);//串口1TX是DMA通道4
	//
	
	DMA_ClearFlag(DMA1_FLAG_TC4);//先将这个标志位清除
	//6、使能DMA
	DMA_Cmd(DMA1_Channel4, ENABLE);
	
}



void USART1_DMA_RX_config(void)
{
	NVIC_InitTypeDef NVIC_InitStructure;
	DMA_InitTypeDef DMA_InitStruct;
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);//使能DMA1通道5的时钟
	
	DMA_InitStruct.DMA_PeripheralBaseAddr=(u32)&USART1->DR;//外设地址（或者USART1_BASE+0x04）
	DMA_InitStruct.DMA_MemoryBaseAddr=(uint32_t)modbus.rcbuf;//存储器地址（指向了数组首地址
	DMA_InitStruct.DMA_DIR=DMA_DIR_PeripheralSRC;//外设作为源地址
	//规定外设是作为数据传输的目的地还是来源（数据传输方向）
	//DMA_DIR_PeripheralDST	外设作为数据传输的目的地
	//DMA_DIR_PeripheralSRC	外设作为数据传输的来源

	DMA_InitStruct.DMA_BufferSize= 8;//传输数目（数组的长度）
	
	//只有一个串口数据寄存器不需要递增，数组是U8类型,所以一次传输一个字节
	DMA_InitStruct.DMA_PeripheralInc=DMA_PeripheralInc_Disable ;//外设地址b不需要递增
	DMA_InitStruct.DMA_PeripheralDataSize= DMA_PeripheralDataSize_Byte  ;//数据宽度，u8类型，一个字节
	//配置memory（定义了一个数组，发送一个会继续下一个，所以地址是递增）
	DMA_InitStruct.DMA_MemoryInc=DMA_MemoryInc_Enable ;//内存地址递增
	DMA_InitStruct.DMA_MemoryDataSize=DMA_MemoryDataSize_Byte  ;//一个字节
	//5、配置模式和优先级
	DMA_InitStruct.DMA_Mode=DMA_Mode_Normal ;//正常模式或者是循环模式
	DMA_InitStruct.DMA_Priority=DMA_Priority_High ;//共有四种优先级
	DMA_InitStruct.DMA_M2M=DMA_M2M_Disable  ;//不使用M-M模式
	DMA_Init(DMA1_Channel5, &DMA_InitStruct);//串口1rX是DMA通道5
	
	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel5_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0 ;//抢占优先级3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;		//子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器
  
	DMA_ClearFlag(DMA1_FLAG_TC5);//先将这个标志位清除0：在通道x没有传输完成事件(TC)；1：在通道x产生了传输完成事件(TC
	DMA_ITConfig(DMA1_Channel5, DMA_IT_TC ,ENABLE);
	//6、使能DMA
	DMA_Cmd(DMA1_Channel5, ENABLE);
	
}

/*以下为基础函数*/

/**
  * 函  数：DMA重装传输数目并使能(dmA接收)，主机寻址从机时发送的字节个数永远是8个字节数据
  * 参  数：无
  * 返回值：无
  */
void DMA_RX_Enable()
{
	//重新装入要发送的字符数并使能
	DMA_Cmd (DMA1_Channel5,DISABLE);//关闭DMA通道
	DMA_ClearFlag(DMA1_FLAG_TC5);//清标志
	DMA_SetCurrDataCounter(DMA1_Channel5,8);//重置传输数目
	DMA_Cmd (DMA1_Channel5,ENABLE);//开启DMA通道

}

/**
  * 函  数：DMA重装传输数目并使能，5+寄存器个数*2
  * 参  数：无
  * 返回值：无
  */
void DMA_TX_Enable(uint8_t num)//num寄存器的个数
{
		DMA_Cmd(DMA1_Channel4,DISABLE);
		DMA_ClearFlag(DMA1_FLAG_TC4);//先将这个接收标志位清除
		DMA_SetCurrDataCounter(DMA1_Channel4, num*2+5); //这是从机返回的字符个数：read_num*2+5
		DMA_Cmd(DMA1_Channel4, ENABLE);
}

/**
  * 函  数：DMA发送数据函数
  * 参  数：无
  * 返回值：无
  */
void DMA_TX_data()
{
	//发送数据
	RS485_TX_ENABLE;//使能485控制端(启动发送) 
	while(DMA_GetFlagStatus(DMA1_FLAG_TC4)==RESET);//如果返回值位reset表示还未传输成功//等待发送完毕
	Delay_ms(5);//如果不加这个延时将丢失最后两个字节数据
	RS485_RX_ENABLE;//开启接收
}

/*以下为从机函数*/

//DMA接收中断
void DMA1_Channel5_IRQHandler()
{
	if(DMA_GetITStatus(DMA1_IT_TC5))
    {
		modbus.reflag = 1;//表明接收数据完毕
		modbus.recount = 8;
        DMA_ClearITPendingBit(DMA1_IT_GL5); //清除全部中断标志
    }
//	LED1_Turn();
	//从机命令接收完毕
//	DMA_ClearITPendingBit(DMA1_IT_TC5);//清除传送完毕标志位
}

//DMA接收中断
void DMA1_Channel4_IRQHandler()
{
	if(DMA_GetITStatus(DMA1_IT_TC4))
    {
//		LED1_Turn();
        DMA_ClearITPendingBit(DMA1_IT_GL4); //清除全部中断标志
    }
//	LED1_Turn();
	//从机命令接收完毕
//	DMA_ClearITPendingBit(DMA1_IT_TC5);//清除传送完毕标志位
}



/*以下为主机函数*/

////这是可用定时器每S调用一次发送数据----使其源源不断的进行发送数据
//void MY_DMA_Enable()
//{
//	
//	HOST_ModbusRX();//数据接收处理函数---处理完毕则modbus.Host_End=1

//		if(modbus.reflag&&modbus.Host_End)//接收数据完毕且数据处理完毕的情况下执行
//		{
//			 modbus.reflag=0;//清空接收标志
//			 modbus.Host_End=0;//清空数据处理完毕标志
//			
//			DMA_TX_Enable();
//				//填写匹配的从机地址，起始地址，寄存器个数
//	//		 Host_Read03_slave(0x03,0x0000,0x0001);//，参数2起始地址，参数3寄存器个数---只是包装发送数组了
//			Host_Read03_slave(find_addr,start_addr,read_num);//，参数2起始地址，参数3寄存器个数--发送数据

//				 	//这是重新装入DMA接收的字符数并使能
//		DMA_Cmd(DMA1_Channel6,DISABLE);
//		DMA_ClearFlag(DMA1_FLAG_TC6);//先将这个接收标志位清除
//		DMA_SetCurrDataCounter(DMA1_Channel6, read_num*2+5); //这是从机返回的字符个数：read_num*2+5
//		DMA_Cmd(DMA1_Channel6, ENABLE);
//		}
//}

////这是遍历读取多个从机的寄存器数值（1s一个）
//void DMA_Data(u8 addr,u16 start_add,u16 num)
//{

//		//处理打印数据完毕时modbus.Host_end=1
//		HOST_ModbusRX();//数据接收处理函数---处理完毕则modbus.Host_End=1
//	
//		if(modbus.reflag&&modbus.Host_End)//重新发送数据//接收数据完毕且数据处理完毕的情况下执行
//		{
//			modbus.reflag=0;//清空接收标志
//			modbus.Host_End=0;//清空数据处理完毕标志
//			
//			//发送重新使能
//			DMA_TX_Enable();//发送重新使能

//			
//			//填充数据
//			Host_Read03_slave(addr,start_add,num);//，参数2起始地址，参数3寄存器个数--发送数据
//			
//			//发送数据
//			DMA_TX_data();
//			
//			
//			//接收重新使能
//			DMA_RX_Enable(num);//重装接收数据个数--num寄存器的个数
//	}
//	
//		
//}

/**
  * 函  数：读取从机的寄存器数据参数设置并发送数据
  * 参  数：slave:从机地址
  * 参  数：StartAddr:起始地址
  * 参  数：num:寄存器个数
  * 返回值：无
  */
//读取从机的寄存器数据参数设置并发送数据
//void Host_read03_set(uint8_t slave,uint16_t StartAddr,uint16_t num)
//{
//		//发送重新使能
//		DMA_TX_Enable();//发送重新使能
//		
//		Host_Read03_slave(slave,StartAddr,num);//填充发送数据的数组内容
//	
//		modbus.Host_End=0;//数据处理完成标志位清零
//		
//		DMA_TX_data();//发送数据
//		//接收重新使能
//		DMA_RX_Enable(num);//重装接收数据个数--num寄存器的个数
//}

/**
  * 函  数：主机填充发送的数据+对从机返回的数据进行处理
  * 参  数：slave:从机地址
  * 参  数：StartAddr:起始地址
  * 参  数：num:寄存器个数
  * 返回值：无
  */
////寻址从机发送指令并对接收的数据处理函数
////参数1从机地址，参数2起始地址，参数3寄存器个数
//void read03(uint8_t slave,uint16_t StartAddr,uint16_t num)
//{
//	
//		//发送数据
//		Host_read03_set(slave,StartAddr,num);
//	    //接收的数据处理
//		if(modbus.Host_End == 0)
//		{
//			HOST_ModbusRX();//进行数据处理
//		}

//}




////参数设置+数据发送
//void Host_write06_set(uint8_t slave,uint8_t fun,uint16_t StartAddr,uint16_t num)
//{
//		//发送重新使能
//		DMA_TX_Enable();//发送重新使能

//		Host_write06_slave(slave,fun,StartAddr,num);
//		modbus.Host_End=0;
//		//发送数据
//		DMA_TX_data();
//		//接收重新使能
//		DMA_RX_Enable(num);//重装接收数据个数--num寄存器的个数

//}

//void read()
//{
//	
//	//发送数据
//	//	Host_read03_set(0x01,0x0000,0x02);
//	 Host_write06_set(0x01,0x06,0x0000,0x0002);
//	//接收的数据处理
//		if(modbus.Host_End!=1)
//		{
//			Host_Func6();//进行数据处理
//		}

//}


