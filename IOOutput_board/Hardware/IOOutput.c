#include "stm32f10x.h"                  // Device header

//输出IO口
uint16_t OutputIO[] = {GPIO_Pin_1,GPIO_Pin_2,GPIO_Pin_3,GPIO_Pin_4,
					  GPIO_Pin_5,GPIO_Pin_6,GPIO_Pin_7,GPIO_Pin_8};
GPIO_TypeDef* OutputType[] = {GPIOA,GPIOA,GPIOA,GPIOA,
							 GPIOA,GPIOA,GPIOA,GPIOA};

void IOOutput_Init(void)
{
	/*开启时钟*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);		//开启GPIOA的时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);		//开启GPIOB的时钟
	
	/*GPIO初始化*/
	GPIO_InitTypeDef GPIO_InitStructure;	
	uint8_t i;
	for(i=0;i<8;i++){
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
		GPIO_InitStructure.GPIO_Pin = OutputIO[i];
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_Init(OutputType[i], &GPIO_InitStructure);						//将PA1和PA2引脚初始化为推挽输出
		/*设置GPIO初始化后的默认电平*/
		GPIO_ResetBits(OutputType[i], OutputIO[i]);				//设置PA1和PA2引脚为高电平
	}
}


