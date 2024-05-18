#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "MyCAN.h"
#include "IOInput.h"
//CAN定义
uint32_t TxID = 0x03;
uint8_t TxLength = 8;
uint8_t TxData[] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08};

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
			if(RxID == 0x03){
				for(i=0;i<8;i++){
					if(GPIO_ReadInputDataBit(InputType[i],InputIO[i])){
						TxData[i] = 0x01;
					}
					else{
						TxData[i] = 0x00;
					}
				}
				MyCAN_Transmit(TxID, TxLength, TxData);
				OLED_ShowNum(1,1,1,1);
			}
		}
		
	}
}
