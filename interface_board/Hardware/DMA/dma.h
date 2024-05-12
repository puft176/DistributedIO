#ifndef _DMA_H
#define _DMA_H
#include "stm32f10x.h"
//#define SendBuff_Size 7

//extern u8 SendBuffe[SendBuff_Size];
void USART1_DMA_RX_config(void);
void USART1_DMA_TX_config(void);

void MY_DMA_Enable(void);

void DMA_Data(u8 addr,u16 start_add,u16 num);

//这是DMA重装传输数目并使能(DMA发送)
void DMA_TX_Enable(uint8_t num);
//这是DMA重装传输数目并使能(DMA接收)
void DMA_RX_Enable(void);
//DMA发送数据函数
void DMA_TX_data(void);
//读取从机的寄存器数据参数设置并发送数据
void Host_read03_set(uint8_t slave,uint16_t StartAddr,uint16_t num);
//寻址从机发送指令并对接收的数据处理函数
//参数1从机地址，参数2起始地址，参数3寄存器个数
void read03(uint8_t slave,uint16_t StartAddr,uint16_t num);
//参数设置+数据发送（向一个寄存器中写入数据）
void Host_write06_set(uint8_t slave,uint8_t fun,uint16_t StartAddr,uint16_t num);

#endif 

