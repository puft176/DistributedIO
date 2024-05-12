#include "modbus.h"
#include "usart.h"
#include "delay.h"
#include "led.h"
#include "dma.h"
MODBUS modbus;//结构体变量


/*作为从机部分的代码*/
//开关量寄存器 0x0000 为OFF，0xFF00 为ON，其他所有值均为非法
u16 Reg0[100] = {0x0000};//    0~ 9999
u16 Reg1[100] = {0x0000};//10000~19999
//16bit寄存器
u16 Reg3[100] ={0x0000};// 30000~39999
u16 Reg4[100] ={0x0000};// 40000~49999
/**
  *@brief Modbus初始化函数
  *@param 无
  *@retval 无
  */
void Modbus_Init()
{
	modbus.myadd = 0x01;  //本机设备地址为1
	modbus.timrun = 0;    //modbus定时器停止计算
	modbus.slave_add=0x01;//作为主机时要匹配的从机地址
}
// Modbus 3号功能码函数
// Modbus 主机读取寄存器值
void Modbus_Func3()
{
    u16 Regadd,Reglen,crc;
	u8 i,j;
	//得到要读取寄存器的首地址 0~65535
	Regadd = modbus.rcbuf[2]*256+modbus.rcbuf[3];//读取的首地址
	//得到要读取寄存器的数据长度 0~65535
	Reglen = modbus.rcbuf[4]*256+modbus.rcbuf[5];//读取的寄存器个数
	//发送回应数据包
	i = 0;
	modbus.sendbuf[i++] = modbus.myadd;      //ID号：发送本机设备地址
	modbus.sendbuf[i++] = 0x03;              //发送功能码
	modbus.sendbuf[i++] = ((Reglen*2)%256);  //返回字节个数
	for(j=0;j<Reglen;j++)					 //返回数据
	{
		//reg是提前定义好的16位数组（模仿寄存器）
		modbus.sendbuf[i++] = Reg4[Regadd+j]/256;//高位数据
		modbus.sendbuf[i++] = Reg4[Regadd+j]%256;//低位数据
	}
	crc = Modbus_CRC16(modbus.sendbuf,i);    //计算要返回数据的CRC
	modbus.sendbuf[i++] = crc/256;//校验位高位
	modbus.sendbuf[i++] = crc%256;//校验位低位
	//数据包打包完成
	// 开始返回Modbus数据
	//发送重新使能
	RS485_TX_ENABLE;//使能485控制端(启动发送) 
	DMA_TX_Enable(1);//发送重新使能,重装发送数据个数,此时数据已经开始发送
	while(DMA_GetFlagStatus(DMA1_FLAG_TC4)==RESET);//如果返回值位reset表示还未传输成功//等待发送完毕
	Delay_ms(5);//如果不加这个延时将丢失最后两个字节数据
	RS485_RX_ENABLE;//开启接收
	//接收重新使能
	DMA_RX_Enable();
}


// Modbus 6号功能码函数
// Modbus 主机写入寄存器值
void Modbus_Func6()  
{
	u16 Regadd;//地址16位
	u16 val;//值
	u16 i,crc,j;
	i=0;
	Regadd=modbus.rcbuf[2]*256+modbus.rcbuf[3];  //得到要修改的地址 
	val=modbus.rcbuf[4]*256+modbus.rcbuf[5];     //修改后的值（要写入的数据）
	Reg4[Regadd]=val;  //修改本设备相应的寄存器
	
	//以下为回应主机
	modbus.sendbuf[i++]=modbus.myadd;//本设备地址
	modbus.sendbuf[i++]=0x06;        //功能码 
	modbus.sendbuf[i++]=Regadd/256;//写入的地址
	modbus.sendbuf[i++]=Regadd%256;
	modbus.sendbuf[i++]=val/256;//写入的数值
	modbus.sendbuf[i++]=val%256;
	crc=Modbus_CRC16(modbus.sendbuf,i);//获取crc校验位
	modbus.sendbuf[i++]=crc/256;  //crc校验位加入包中
	modbus.sendbuf[i++]=crc%256;
	//数据发送包打包完毕
	RS485_TX_ENABLE;;//使能485控制端(启动发送)  
	for(j=0;j<i;j++)
	{
	 Usart_SendByte(USART1,modbus.sendbuf[j]);
	}
	RS485_RX_ENABLE;//失能485控制端（改为接收）
}

//这是往多个寄存器器中写入数据
//功能码0x10指令即十进制16
void Modbus_Func16()
{
		u16 Regadd;//地址16位
		u16 Reglen;
		u16 i,crc,j;
		
		Regadd=modbus.rcbuf[2]*256+modbus.rcbuf[3];  //要修改内容的起始地址
		Reglen = modbus.rcbuf[4]*256+modbus.rcbuf[5];//读取的寄存器个数
		for(i=0;i<Reglen;i++)//往寄存器中写入数据
		{
			//接收数组的第七位开始是数据
			Reg4[Regadd+i]=modbus.rcbuf[7+i*2]*256+modbus.rcbuf[8+i*2];//对寄存器一次写入数据
		}
		//写入数据完毕，接下来需要进行打包回复数据了
		
		//以下为回应主机内容
		//内容=接收数组的前6位+两位的校验位
		modbus.sendbuf[0]=modbus.rcbuf[0];//本设备地址
		modbus.sendbuf[1]=modbus.rcbuf[1];  //功能码 
		modbus.sendbuf[2]=modbus.rcbuf[2];//写入的地址
		modbus.sendbuf[3]=modbus.rcbuf[3];
		modbus.sendbuf[4]=modbus.rcbuf[4];
		modbus.sendbuf[5]=modbus.rcbuf[5];
		crc=Modbus_CRC16(modbus.sendbuf,6);//获取crc校验位
		modbus.sendbuf[6]=crc/256;  //crc校验位加入包中
		modbus.sendbuf[7]=crc%256;
		//数据发送包打包完毕
		
		RS485_TX_ENABLE;;//使能485控制端(启动发送)  
		for(j=0;j<8;j++)
		{
			Usart_SendByte(USART1,modbus.sendbuf[j]);
		}
		RS485_RX_ENABLE;//失能485控制端（改为接收）
}

/*****************************
Modbus事件处理函数
只有当数据接收完毕时才进行数据处理
1. 首先判断自主计算的CRC校验位和接收到数据的校验位是否一致
2. 其次判断从机地址是不是自己的地址
3. 数据传输正确且从机地址正确的情况下再根据不同的功能码去执行对应的函数操作
0x03 读取寄存器数据
0x06 写入寄存器数据
0x10 写入多个寄存器数据
******************************/
void Modbus_Event()
{
	u16 crc,rccrc;//crc和接收到的crc
	//没有收到数据包
	if(modbus.reflag == 0)  //如果接收未完成则返回空
	{
	   return;
	}
	//收到数据包(接收完成)
	//通过读到的数据帧计算CRC
	//参数1是数组首地址，参数2是要计算的长度（除了CRC校验位其余全算）
//	LED1_ON();
	crc = Modbus_CRC16(&modbus.rcbuf[0],modbus.recount-2); //获取CRC校验位
	// 读取数据帧的CRC
	rccrc = modbus.rcbuf[modbus.recount-2]*256+modbus.rcbuf[modbus.recount-1];//计算读取的CRC校验位
	//等价于下面这条语句
	//rccrc=modbus.rcbuf[modbus.recount-1]|(((u16)modbus.rcbuf[modbus.recount-2])<<8);//获取接收到的CRC
	if(crc == rccrc) //CRC检验成功 开始分析包
	{	
//		LED1_ON();
	   if(modbus.rcbuf[0] == modbus.myadd)  // 检查地址是否时自己的地址
		 {
			 
		   switch(modbus.rcbuf[1])   //分析modbus功能码
			 {
				 case 1:             break;
				 case 2:             break;
				 case 3:      Modbus_Func3();LED1_ON();      break;//这是读取寄存器的数据
				 case 4:             break;
				 case 5:             break;
				 case 6:      Modbus_Func6();      break;//这是写入单个寄存器数据
				 case 8:             break;
				 case 15:     		 break;
				 case 16:     Modbus_Func16(); 			break;//写入多个寄存器数据
			 }
		 }
		 else if(modbus.rcbuf[0] == 0) //广播地址不予回应
		 {
		    
		 }	 
	}
//	modbus.recount = 0;//接收计数清零
    modbus.reflag = 0; //接收标志清零
}
//作为从机部分内容结束


/*作为主机时内容*/

/**
  *@brief 功能码0x03 主机从指定从机读取数据
  *@param slave: 从机编号
  *@param StartAddr: 要读取的寄存器起始地址0~65535
  *@param num:要读取的寄存器个数0~65535
  *@retval 无
  */
void Host_Read03_slave(uint8_t slave,uint16_t StartAddr,uint16_t num)
{
	int j;
	u16 crc;									//计算的CRC校验位
	modbus.slave_add=slave;
	modbus.Host_Txbuf[0]=slave;					//要匹配的从机地址
	modbus.Host_Txbuf[1]=0x03;					//功能码
	modbus.Host_Txbuf[2]=StartAddr/256;			//起始地址高位
	modbus.Host_Txbuf[3]=StartAddr%256;			//起始地址低位
	modbus.Host_Txbuf[4]=num/256;				//寄存器个数高位
	modbus.Host_Txbuf[5]=num%256;				//寄存器个数低位
	crc=Modbus_CRC16(&modbus.Host_Txbuf[0],6); //获取CRC校验位
	modbus.Host_Txbuf[6]=crc/256;//寄存器个数高位	？
	modbus.Host_Txbuf[7]=crc%256;//寄存器个数低位
	
	/***************
	使用DMA转运时不再需要手动将数据送到串口进行发送
	//发送数据包装完毕
	RS485_TX_ENABLE;//使能485控制端(启动发送) 
	for(j=0;j<8;j++)
	{
		Modbus_Send_Byte(modbus.Host_Txbuf[j]);
	}
	RS485_RX_ENABLE;//失能485控制端（改为接收）
	modbus.Host_send_flag=1;//表示发送数据完毕
	****************/
}

//第三个是字节个数
//主机处理接受到的数据
/**
  *@brief 主机接收从机返回的数据
  *@param 无
  *@retval 无
  */
void Host_Func3()
{
	int i;
	int count=(int)modbus.rcbuf[2];//这是数据个数
	
	printf("从机返回 %d 个寄存器数据：\r\n",count/2);
	for(i=0;i<count;i=i+2)
	{
		printf("Temp_Hbit= %d Temp_Lbit= %d temp= %d\r\n",(int)modbus.rcbuf[3+i],(int)modbus.rcbuf[4+i],(int)modbus.rcbuf[4+i]+((int)modbus.rcbuf[3+i])*256);
	}
	modbus.Host_End=1;//接收的数据处理完毕
}

//向一个寄存器中写数据的参数设置
void Host_write06_slave(uint8_t slave,uint8_t fun,uint16_t StartAddr,uint16_t num)
{
	u16 crc;//计算的CRC校验位
	modbus.slave_add=slave;//从机地址赋值一下，后期有用
	modbus.Host_Txbuf[0]=slave;//这是要匹配的从机地址
	modbus.Host_Txbuf[1]=fun;//功能码
	modbus.Host_Txbuf[2]=StartAddr/256;//起始地址高位
	modbus.Host_Txbuf[3]=StartAddr%256;//起始地址低位
	modbus.Host_Txbuf[4]=num/256;
	modbus.Host_Txbuf[5]=num%256;
	crc=Modbus_CRC16(&modbus.Host_Txbuf[0],6); //获取CRC校验位
	modbus.Host_Txbuf[6]=crc/256;//寄存器个数高位
	modbus.Host_Txbuf[7]=crc%256;//寄存器个数低位

}


//主机接收从机的消息进行处理
void HOST_ModbusRX()
{
	u16 crc,rccrc;//计算crc和接收到的crc

    if(modbus.reflag == 0)  //如果接收未完成则返回空
	{
	   return;
	}
	//接收数据结束
	
	//（数组中除了最后两位CRC校验位其余全算）
	crc = Modbus_CRC16(&modbus.rcbuf[0],modbus.recount-2); //获取CRC校验位
	rccrc = modbus.rcbuf[modbus.recount-2]*256+modbus.rcbuf[modbus.recount-1];//计算读取的CRC校验位
	
	if(crc == rccrc) //CRC检验成功 开始分析包
	{	
	   if(modbus.rcbuf[0] == modbus.slave_add)  // 检查地址是是对应从机发过来的
		 {
			 if(modbus.rcbuf[1]==3)//功能码时03
		      Host_Func3();//这是读取寄存器的有效数据位进行计算
		 }
		 
	}	
//	 modbus.recount = 0;//接收计数清零
   modbus.reflag = 0; //接收标志清零
	
}

//modbus串口中断服务程序
void USART1_IRQHandler(void)                
{
//    u8 Res;
	if(USART_GetITStatus(USART1, USART_IT_RXNE) == SET)//接收中断
	{
//		Res =USART_ReceiveData(USART1);	//读取接收到的数据
////	    USART_SendData(USART1, Res);//接受到数据之后返回给串口1
//		if(modbus.reflag==1)  //上一个数据包接收完毕正在处理
//		{
//			return ;
//		}
//		modbus.rcbuf[modbus.recount++] = Res;//recount: modbus端口接收到的数据个数
//		modbus.timout = 0;					 //timout: modbus数据持续时间
//		if(modbus.recount == 1)  //已经收到了第二个字符数据
//		{
//			modbus.timrun = 1;  //开启modbus定时器计时
//		}
//		LED1_ON();
	}	
} 

