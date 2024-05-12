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

#endif 

