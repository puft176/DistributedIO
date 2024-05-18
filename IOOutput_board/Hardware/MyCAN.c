#include "stm32f10x.h"                  // Device header

void MyCAN_Init(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	CAN_InitTypeDef CAN_InitStructure;
	CAN_InitStructure.CAN_Prescaler = 48;			//预分频，范围(1~1024)
	CAN_InitStructure.CAN_Mode = CAN_Mode_Normal;	//正常工作模式
	CAN_InitStructure.CAN_SJW = CAN_SJW_1tq;		//再次同步跳跃宽度为1Tq(设置再次同步补偿宽度，因时钟频率偏差、传送延迟等，各单元有同步误差，这里设置补偿此误差的最大值，范围为1~4Tq；)
	CAN_InitStructure.CAN_BS1 = CAN_BS1_2tq;		//位段1(BS1)的长度为2Tq
	CAN_InitStructure.CAN_BS2 = CAN_BS2_3tq;		//位段2(BS2)的长度为3Tq
	CAN_InitStructure.CAN_TTCM = DISABLE;			//禁止时间触发通信模式time triggered mode
	CAN_InitStructure.CAN_ABOM = DISABLE;			//禁止总线自动关闭 auto bus off
	CAN_InitStructure.CAN_AWUM = DISABLE;			//禁止自动唤醒 auto wake up
	CAN_InitStructure.CAN_NART = DISABLE;			//使能自动重传 auto retransmission
	CAN_InitStructure.CAN_RFLM = DISABLE;			//禁止接收FIFO锁定 receive FIFO locked
	CAN_InitStructure.CAN_TXFP = DISABLE;			//禁止传输FIFO优先级 transmit FIFO priority
	CAN_Init(CAN1, &CAN_InitStructure);
	/*配置CAN的筛选器，此处全部接收，不做过滤*/
	CAN_FilterInitTypeDef CAN_FilterInitStructure;
	
	CAN_FilterInitStructure.CAN_FilterIdHigh = 0x0000;					//ID高字节
	CAN_FilterInitStructure.CAN_FilterIdLow = 0x0000;					//ID低字节
	CAN_FilterInitStructure.CAN_FilterMaskIdHigh = 0x0000;				//掩码高字节
	CAN_FilterInitStructure.CAN_FilterMaskIdLow = 0x0000;				//掩码低字节	
	CAN_FilterInitStructure.CAN_FilterFIFOAssignment = CAN_Filter_FIFO0;//过滤器关联FIFO
	CAN_FilterInitStructure.CAN_FilterNumber = 0;						//选择筛选器组0
	CAN_FilterInitStructure.CAN_FilterMode = CAN_FilterMode_IdMask;		//设置为掩码模式	
	CAN_FilterInitStructure.CAN_FilterScale = CAN_FilterScale_32bit;	//32位长度
	CAN_FilterInitStructure.CAN_FilterActivation = ENABLE;				//使能筛选器
	CAN_FilterInit(&CAN_FilterInitStructure);
}


/**
  * 函  数：can1发送函数
  * 参  数：ID：can发送报文的ID
  * 参  数：Length：发送报文的字节数（0~8）
  * 参  数：*Data发送报文的首地址
  * 返回值：无
  */
void MyCAN_Transmit(uint32_t ID, uint8_t Length, uint8_t *Data)
{
	CanTxMsg TxMessage;
	TxMessage.StdId = ID;			//标准标识符
	TxMessage.ExtId = ID;			//拓展标识符
	TxMessage.IDE = CAN_Id_Standard;//帧模式（标准帧或拓展帧）
	TxMessage.RTR = CAN_RTR_Data;	//帧类型（数据帧或遥控帧）
	TxMessage.DLC = Length;			//数据长度
	for (uint8_t i = 0; i < Length; i ++)
	{
		TxMessage.Data[i] = Data[i];
	}
	uint8_t TransmitMailbox = CAN_Transmit(CAN1, &TxMessage);
	while (CAN_TransmitStatus(CAN1, TransmitMailbox) != CAN_TxStatus_Ok);
}

/**
  * 函  数：can1遥控函数
  * 参  数：ID：can发送报文的ID
  * 参  数：Length：请求的数据的字节数（0~8）
  * 返回值：无
  */
void MyCAN_ASK(uint32_t ID, uint8_t Length)
{
	CanTxMsg TxMessage;
	TxMessage.StdId = ID;			//标准标识符
	TxMessage.ExtId = ID;			//拓展标识符
	TxMessage.IDE = CAN_Id_Standard;//帧模式（标准帧或拓展帧）
	TxMessage.RTR = CAN_RTR_Remote;	//帧类型（数据帧或遥控帧）
	TxMessage.DLC = Length;			//数据长度
	uint8_t TransmitMailbox = CAN_Transmit(CAN1, &TxMessage);
	while (CAN_TransmitStatus(CAN1, TransmitMailbox) != CAN_TxStatus_Ok);
}


uint8_t MyCAN_ReceiveFlag(void)
{
	if (CAN_MessagePending(CAN1, CAN_FIFO0) > 0)
	{
		return 1;
	}
	return 0;
}

void MyCAN_Receive(uint32_t *ID, uint8_t *Length, uint8_t *Data)
{
	CanRxMsg RxMessage;
	
	CAN_Receive(CAN1, CAN_FIFO0, &RxMessage);
	
	if (RxMessage.IDE == CAN_Id_Standard)
	{
		*ID = RxMessage.StdId;
	}
	else
	{
		//...
	}
	
	if (RxMessage.RTR == CAN_RTR_Data)
	{
		*Length = RxMessage.DLC;
		for (uint8_t i = 0; i < *Length; i ++)
		{
			Data[i] = RxMessage.Data[i];
		}
	}
	else if(RxMessage.RTR == CAN_RTR_Remote){
		//...
	}
	else
	{
		//...
	}
}
