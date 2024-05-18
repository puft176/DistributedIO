#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "MyCAN.h"
#include "IOOutput.h"

uint32_t TxID = 0x02;
uint8_t TxLength = 8;
uint8_t TxData[] = {0x00};

uint32_t RxID;
uint8_t RxLength;
uint8_t RxData[8];

int main(void)
{
	/*模块初始化*/
	OLED_Init();		//OLED初始化
	MyCAN_Init();
	IOOutput_Init();
	uint8_t i;
	while (1)
	{
		if (MyCAN_ReceiveFlag())
		{
			MyCAN_Receive(&RxID, &RxLength, RxData);
			if(RxID == 0x02){	//判断是否是数字量输出指令
				for(i=0;i<8;i++){	//遍历8位输出
					if(RxData[i]){
						GPIO_SetBits(OutputType[i],OutputIO[i]);
					}
					else{
						GPIO_ResetBits(OutputType[i],OutputIO[i]);
					}
				}
			}
		}
		
	}
}
