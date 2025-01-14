#include "modbus.h"
#include "usart.h"
#include "delay.h"
#include "led.h"
#include "dma.h"
#include "MyCAN.h"
MODBUS modbus;//结构体变量

uint32_t TxID = 0x01;
uint8_t TxLength = 8;
uint8_t TxData[] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08};

uint32_t RxID;
uint8_t RxLength;
uint8_t RxData[8];

/*作为从机部分的代码*/
//开关量寄存器 0x0000 为OFF，0xFF00 为ON，其他所有值均为非法
u16 Reg0[100] = {0x0000};//    0~ 9999 Q区
u16 Reg1[100] = {0x0000};//10000~19999 I区
//16bit寄存器
u16 Reg3[100] ={0x0000};// 30000~39999 
u16 Reg4[100] ={0x0000};// 40000~49999 M区
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
	modbus.interval = 1000;//报文收发间隔
}

/*以下为公共功能码*/

/**
  * 函  数：Modbus 1号功能码函数，对1-9999地址的随机读访问
  * 参  数：无
  * 返回值：无
  */
void Modbus_Func1()
{
    u16 Regadd,Reglen,crc;
	u8 i,j;
	//得到要读取寄存器的首地址 0~65535
	Regadd = modbus.rcbuf2[2]*256+modbus.rcbuf2[3];//读取的首地址
	//得到要读取寄存器的数据长度 0~65535
	Reglen = modbus.rcbuf2[4]*256+modbus.rcbuf2[5];//读取的寄存器个数
	//发送回应数据包
	i = 0;
	modbus.sendbuf[i++] = modbus.myadd;      //ID号：发送本机设备地址
	modbus.sendbuf[i++] = 0x02;              //发送功能码
	modbus.sendbuf[i++] = ((Reglen*2)%256);  //返回字节个数
	for(j=0;j<Reglen;j++)					 //返回数据
	{
		//reg是提前定义好的16位数组（模仿寄存器）
		modbus.sendbuf[i++] = Reg0[Regadd+j]/256;//高位数据
		modbus.sendbuf[i++] = Reg0[Regadd+j]%256;//低位数据
	}
	crc = Modbus_CRC16(modbus.sendbuf,i);    //计算要返回数据的CRC
	modbus.sendbuf[i++] = crc/256;//校验位高位
	modbus.sendbuf[i++] = crc%256;//校验位低位
	//数据包打包完成
	// 开始返回Modbus数据
	while(modbus.time_flag == 0);//等待设定的收发间隔
	//发送重新使能
	RS485_TX_ENABLE;								//使能485控制端(启动发送)
	DMA_TX_Enable(5+2*Reglen);						//发送重新使能,重装发送数据个数,此时数据已经开始发送
	while(DMA_GetFlagStatus(DMA1_FLAG_TC4)==RESET);	//如果返回值位reset表示还未传输成功//等待发送完毕
//	Delay_ms(5);									//如果不加这个延时将丢失最后两个字节数据（实验后发现没有丢失，后续有需要可以去掉）
	RS485_RX_ENABLE;								//开启接收
	//接收重新使能
	DMA_RX_Enable();
}

/**
  * 函  数：Modbus 2号功能码函数，对10001-19999地址的随机读访问
  * 参  数：无
  * 返回值：无
  */
void Modbus_Func2()
{
    u16 Regadd,Reglen,crc;
	u8 i,j;
	MyCAN_ASK(0x03,8);	//向数字量输入板请求数据
	
	while(MyCAN_ReceiveFlag() == 0);	//等待数字量输入板的数据
	
	MyCAN_Receive(&RxID, &RxLength, RxData);	//接收数据
	for(i=0;i<8;i++){	//将数据写入寄存器
		if(RxData[i]){
			Reg1[i+1] = 0xFF00;
		}
		else{
			Reg1[i+1] = 0x0000;
		}
	}
	
	//得到要读取寄存器的首地址 0~65535
	Regadd = modbus.rcbuf2[2]*256+modbus.rcbuf2[3];//读取的首地址
	//得到要读取寄存器的数据长度 0~65535
	Reglen = modbus.rcbuf2[4]*256+modbus.rcbuf2[5];//读取的寄存器个数
	
	//发送回应数据包
	i = 0;
	modbus.sendbuf[i++] = modbus.myadd;      //ID号：发送本机设备地址
	modbus.sendbuf[i++] = 0x02;              //发送功能码
	modbus.sendbuf[i++] = ((Reglen*2)%256);  //返回字节个数
	for(j=0;j<Reglen;j++)					 //返回数据
	{
		//reg是提前定义好的16位数组（模仿寄存器）
		modbus.sendbuf[i++] = Reg1[Regadd+j]/256;//高位数据
		modbus.sendbuf[i++] = Reg1[Regadd+j]%256;//低位数据
	}
	crc = Modbus_CRC16(modbus.sendbuf,i);    //计算要返回数据的CRC
	modbus.sendbuf[i++] = crc/256;//校验位高位
	modbus.sendbuf[i++] = crc%256;//校验位低位
	//数据包打包完成
	// 开始返回Modbus数据
	while(modbus.time_flag == 0);//等待设定的收发间隔
	//发送重新使能
	RS485_TX_ENABLE;//使能485控制端(启动发送) 
	DMA_TX_Enable(5+2*Reglen);//发送重新使能,重装发送数据个数,此时数据已经开始发送
	while(DMA_GetFlagStatus(DMA1_FLAG_TC4)==RESET);//如果返回值位reset表示还未传输成功//等待发送完毕
//	Delay_ms(5);//如果不加这个延时将丢失最后两个字节数据（实验后发现没有丢失，后续有需要可以去掉）
	RS485_RX_ENABLE;//开启接收
	//接收重新使能
	DMA_RX_Enable();
}

/**
  * 函  数：Modbus 3号功能码函数，对40001-49999地址的随机读访问
  * 参  数：无
  * 返回值：无
  */
void Modbus_Func3()
{
    u16 Regadd,Reglen,crc;
	u8 i,j;
	//得到要读取寄存器的首地址 0~65535
	Regadd = modbus.rcbuf2[2]*256+modbus.rcbuf2[3];//读取的首地址
	//得到要读取寄存器的数据长度 0~65535
	Reglen = modbus.rcbuf2[4]*256+modbus.rcbuf2[5];//读取的寄存器个数
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
	while(modbus.time_flag == 0);//等待设定的收发间隔
	//发送重新使能
	RS485_TX_ENABLE;//使能485控制端(启动发送) 
	DMA_TX_Enable(5+2*Reglen);//发送重新使能,重装发送数据个数,此时数据已经开始发送
	while(DMA_GetFlagStatus(DMA1_FLAG_TC4)==RESET);//如果返回值位reset表示还未传输成功//等待发送完毕
//	Delay_ms(5);//如果不加这个延时将丢失最后两个字节数据（实验后发现没有丢失，后续有需要可以去掉）
	RS485_RX_ENABLE;//开启接收
	//接收重新使能
	DMA_RX_Enable();
}

/**
  * 函  数：Modbus 4号功能码函数，对30001-39999地址的随机读访问
  * 参  数：无
  * 返回值：无
  */
void Modbus_Func4()
{
    u16 Regadd,Reglen,crc;
	u8 i,j;
	//得到要读取寄存器的首地址 0~65535
	Regadd = modbus.rcbuf2[2]*256+modbus.rcbuf2[3];//读取的首地址
	//得到要读取寄存器的数据长度 0~65535
	Reglen = modbus.rcbuf2[4]*256+modbus.rcbuf2[5];//读取的寄存器个数
	//发送回应数据包
	i = 0;
	modbus.sendbuf[i++] = modbus.myadd;      //ID号：发送本机设备地址
	modbus.sendbuf[i++] = 0x04;              //发送功能码
	modbus.sendbuf[i++] = ((Reglen*2)%256);  //返回字节个数
	for(j=0;j<Reglen;j++)					 //返回数据
	{
		//reg是提前定义好的16位数组（模仿寄存器）
		modbus.sendbuf[i++] = Reg3[Regadd+j]/256;//高位数据
		modbus.sendbuf[i++] = Reg3[Regadd+j]%256;//低位数据
	}
	crc = Modbus_CRC16(modbus.sendbuf,i);    //计算要返回数据的CRC
	modbus.sendbuf[i++] = crc/256;//校验位高位
	modbus.sendbuf[i++] = crc%256;//校验位低位
	//数据包打包完成
	// 开始返回Modbus数据
	while(modbus.time_flag == 0);//等待设定的收发间隔
	//发送重新使能
	RS485_TX_ENABLE;//使能485控制端(启动发送) 
	DMA_TX_Enable(5+2*Reglen);//发送重新使能,重装发送数据个数,此时数据已经开始发送
	while(DMA_GetFlagStatus(DMA1_FLAG_TC4)==RESET);//如果返回值位reset表示还未传输成功//等待发送完毕
//	Delay_ms(5);//如果不加这个延时将丢失最后两个字节数据（实验后发现没有丢失，后续有需要可以去掉）
	RS485_RX_ENABLE;//开启接收
	//接收重新使能
	DMA_RX_Enable();
}

/**
  * 函  数：Modbus 5号功能码函数，对1-9999地址的随机写访问
  * 参  数：无
  * 返回值：无
  */
void Modbus_Func5()
{
	u16 Regadd; //地址16位
	u16 val;	//值
	u16 i,crc;
	Regadd=modbus.rcbuf2[2]*256+modbus.rcbuf2[3];  //得到要修改的地址 
	val=modbus.rcbuf2[4]*256+modbus.rcbuf2[5];     //修改后的值（要写入的数据）
	Reg0[Regadd]=val;  //修改本设备相应的寄存器
	
	/*将输出命令发送给输出板*/
	for(i=0;i<8;i++){
		if(Reg0[i+1]){
			TxData[i] = 0x01;
		}
		else{
			TxData[i] = 0x00;
		}
		
	}
	MyCAN_Transmit(0x02,TxLength,TxData);
	
	
	//以下为回应主机
	i=0;
	modbus.sendbuf[i++]=modbus.myadd;//本设备地址
	modbus.sendbuf[i++]=0x05;        //功能码 
	modbus.sendbuf[i++]=Regadd/256;  //写入的地址
	modbus.sendbuf[i++]=Regadd%256;
	modbus.sendbuf[i++]=val/256;	 //写入的数值
	modbus.sendbuf[i++]=val%256;
	crc=Modbus_CRC16(modbus.sendbuf,i);//获取crc校验位
	modbus.sendbuf[i++]=crc/256;  	   //crc校验位加入包中
	modbus.sendbuf[i++]=crc%256;
	//数据发送包打包完毕
	while(modbus.time_flag == 0);//等待设定的收发间隔
	
	RS485_TX_ENABLE;;	//使能485控制端(启动发送) 
	DMA_TX_Enable(8);	//发送重新使能,重装发送数据个数,此时数据已经开始发送
	while(DMA_GetFlagStatus(DMA1_FLAG_TC4)==RESET);//如果返回值位reset表示还未传输成功//等待发送完毕
//	Delay_ms(5);		//如果不加这个延时将丢失最后两个字节数据（实验后发现没有丢失，后续有需要可以去掉）
	RS485_RX_ENABLE;	//失能485控制端（改为接收）
	//接收重新使能
	DMA_RX_Enable();
}

/**
  * 函  数：Modbus 6号功能码函数，对40001-49999地址的随机写访问
  * 参  数：无
  * 返回值：无
  */
void Modbus_Func6()
{
	u16 Regadd; //地址16位
	u16 val;	//值
	u16 i,crc;
	u32 bound = 0;
	i=0;
	Regadd=modbus.rcbuf2[2]*256+modbus.rcbuf2[3];  //得到要修改的地址 
	val=modbus.rcbuf2[4]*256+modbus.rcbuf2[5];     //修改后的值（要写入的数据）
	Reg4[Regadd]=val;  //修改本设备相应的寄存器
	
	/*功能判断/执行*/
	//波特率修改，40001寄存器
	if(Regadd == 1){
		switch(val)
		{
			case 1: bound = 4800;	break; 
			case 2: bound = 9600;	break; 
			case 3: bound = 38400;	break; 
			case 4: bound = 115200;	break;
		}
	}
	//收发报文间隔时间修改,40002寄存器
	else if(Regadd == 2){
		modbus.interval = val;
	}
	
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
	while(modbus.time_flag == 0);//等待设定的收发间隔
	
	RS485_TX_ENABLE;;	//使能485控制端(启动发送) 
	DMA_TX_Enable(8);	//发送重新使能,重装发送数据个数,此时数据已经开始发送
	while(DMA_GetFlagStatus(DMA1_FLAG_TC4)==RESET);//如果返回值位reset表示还未传输成功//等待发送完毕
//	Delay_ms(5);		//如果不加这个延时将丢失最后两个字节数据（实验后发现没有丢失，后续有需要可以去掉）
	RS485_RX_ENABLE;	//失能485控制端（改为接收）
	//接收重新使能
	DMA_RX_Enable();
	
	/*功能执行*/
	//修改波特率
	if(bound){
		Delay_ms(5);
		NVIC_Configuration(); 	 					//设置NVIC中断分组2:2位抢占优先级，2位响应优先级
		uart_init(bound);							//串口1初始化为9600（只用来打印测试数据）
		USART_DMACmd(USART1,USART_DMAReq_Tx,ENABLE);//开启使能
		USART_DMACmd(USART1,USART_DMAReq_Rx,ENABLE);//开启使能
	}
}

/**
  * 函  数：功能码15 0x0F，对1-9999地址的随机写访问，访问长度2-120
  * 参  数：无
  * 返回值：无
  */
void Modbus_Func15()
{
	u16 Regadd;//地址16位
	u16 Reglen;
	u16 i,crc;
	
	
	Regadd=modbus.rcbuf2[2]*256+modbus.rcbuf2[3];  //要修改内容的起始地址
	Reglen = modbus.rcbuf2[4]*256+modbus.rcbuf2[5];//读取的寄存器个数
	for(i=0;i<Reglen;i++)//往寄存器中写入数据
	{
		//接收数组的第七位开始是数据
		Reg0[Regadd+i]=modbus.rcbuf2[7+i*2]*256+modbus.rcbuf2[8+i*2];//对寄存器一次写入数据
	}
	
	/*将输出命令发送给输出板*/
	for(i=0;i<8;i++){
		if(Reg0[i+1]){
			TxData[i] = 0x01;
		}
		else{
			TxData[i] = 0x00;
		}
		
	}
	MyCAN_Transmit(0x02,TxLength,TxData);
	
	//写入数据完毕，接下来需要进行打包回复数据了
	
	//以下为回应主机内容
	//内容=接收数组的前6位+两位的校验位
	modbus.sendbuf[0]=modbus.rcbuf2[0];//本设备地址
	modbus.sendbuf[1]=modbus.rcbuf2[1];  //功能码 
	modbus.sendbuf[2]=modbus.rcbuf2[2];//写入的地址
	modbus.sendbuf[3]=modbus.rcbuf2[3];
	modbus.sendbuf[4]=modbus.rcbuf2[4];
	modbus.sendbuf[5]=modbus.rcbuf2[5];
	crc=Modbus_CRC16(modbus.sendbuf,6);//获取crc校验位
	modbus.sendbuf[6]=crc/256;  //crc校验位加入包中
	modbus.sendbuf[7]=crc%256;
	//数据发送包打包完毕
	while(modbus.time_flag == 0);//等待设定的收发间隔
	
	RS485_TX_ENABLE;		//使能485控制端(启动发送) 
	DMA_TX_Enable(8);		//发送重新使能,重装发送数据个数,此时数据已经开始发送
	while(DMA_GetFlagStatus(DMA1_FLAG_TC4)==RESET);//如果返回值位reset表示还未传输成功//等待发送完毕
//	Delay_ms(5);			//如果不加这个延时将丢失最后两个字节数据（实验后发现没有丢失，后续有需要可以去掉）
	RS485_RX_ENABLE;		//失能485控制端（改为接收）
	//接收重新使能
	DMA_RX_Enable();
	
}

/**
  * 函  数：功能码16 0x10，对40001-49999地址的随机写访问，访问长度2-120
  * 参  数：无
  * 返回值：无
  */
void Modbus_Func16()
{
	u16 Regadd;//地址16位
	u16 Reglen;
	u16 i,crc;
	u16 val;
	u32 bound = 0;
	
	Regadd=modbus.rcbuf2[2]*256+modbus.rcbuf2[3];  //要修改内容的起始地址
	Reglen = modbus.rcbuf2[4]*256+modbus.rcbuf2[5];//读取的寄存器个数
	for(i=0;i<Reglen;i++)//往寄存器中写入数据
	{
		//接收数组的第七位开始是数据
		val=modbus.rcbuf2[7+i*2]*256+modbus.rcbuf2[8+i*2];//对寄存器一次写入数据
		Reg4[Regadd+i] = val;
		
		/*功能判断/执行*/
		//波特率修改，40001寄存器
		if(Regadd+i == 1){
			switch(val)
			{
				case 1: bound = 4800;	break; 
				case 2: bound = 9600;	break; 
				case 3: bound = 38400;	break; 
				case 4: bound = 115200;	break;
			}
		}
		//收发报文间隔时间修改,40002寄存器
		if(Regadd+i == 2){
			modbus.interval = val;
		}
	}
	//写入数据完毕，接下来需要进行打包回复数据了
	
	/*以下为回应主机内容*/
	//内容=接收数组的前6位+两位的校验位
	modbus.sendbuf[0]=modbus.rcbuf2[0];//本设备地址
	modbus.sendbuf[1]=modbus.rcbuf2[1];  //功能码 
	modbus.sendbuf[2]=modbus.rcbuf2[2];//写入的地址
	modbus.sendbuf[3]=modbus.rcbuf2[3];
	modbus.sendbuf[4]=modbus.rcbuf2[4];
	modbus.sendbuf[5]=modbus.rcbuf2[5];
	crc=Modbus_CRC16(modbus.sendbuf,6);//获取crc校验位
	modbus.sendbuf[6]=crc/256;  //crc校验位加入包中
	modbus.sendbuf[7]=crc%256;
	//数据发送包打包完毕
	while(modbus.time_flag == 0);//等待设定的收发间隔
	
	RS485_TX_ENABLE;		//使能485控制端(启动发送) 
	DMA_TX_Enable(8);		//发送重新使能,重装发送数据个数,此时数据已经开始发送
	while(DMA_GetFlagStatus(DMA1_FLAG_TC4)==RESET);//如果返回值位reset表示还未传输成功//等待发送完毕
//	Delay_ms(5);			//如果不加这个延时将丢失最后两个字节数据（实验后发现没有丢失，后续有需要可以去掉）
	RS485_RX_ENABLE;		//失能485控制端（改为接收）
	//接收重新使能
	DMA_RX_Enable();
	
	/*功能执行*/
	//修改波特率
	if(bound){
		NVIC_Configuration(); 	 					//设置NVIC中断分组2:2位抢占优先级，2位响应优先级
		uart_init(bound);							//串口1初始化为9600（只用来打印测试数据）
		USART_DMACmd(USART1,USART_DMAReq_Tx,ENABLE);//开启使能
		USART_DMACmd(USART1,USART_DMAReq_Rx,ENABLE);//开启使能
	}	
}

/*以下为自定义功能码*/


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
	crc = Modbus_CRC16(&modbus.rcbuf2[0],modbus.recount-2); //获取CRC校验位
	// 读取数据帧的CRC
	rccrc = modbus.rcbuf2[modbus.recount-2]*256+modbus.rcbuf2[modbus.recount-1];//计算读取的CRC校验位
	//等价于下面这条语句
	//rccrc=modbus.rcbuf[modbus.recount-1]|(((u16)modbus.rcbuf[modbus.recount-2])<<8);//获取接收到的CRC
	if(crc == rccrc) //CRC检验成功 开始分析包
	{	
	   if(modbus.rcbuf[0] == modbus.myadd)  // 检查地址是否时自己的地址
		 {
			 
		   switch(modbus.rcbuf[1])   //分析modbus功能码
			 {
				 case 1:      Modbus_Func1();		break;
				 case 2:      Modbus_Func2();		break;
				 case 3:      Modbus_Func3();		break;//这是读取寄存器的数据
				 case 4:      Modbus_Func4();		break;
				 case 5:      Modbus_Func5();       break;
				 case 6:      Modbus_Func6();		break;//这是写入单个寄存器数据
				 case 8:             break;
				 case 15:     Modbus_Func15();		 break;
				 case 16:     Modbus_Func16();		break;//写入多个寄存器数据
			 }
		 }
		 else if(modbus.rcbuf2[0] == 0) //广播地址不予回应
		 {
		    
		 }	 
	}
	modbus.recount = 0;//接收计数清零
    modbus.reflag = 0; //接收标志清零
	modbus.Sendtime = 0;//接收时间计数清零
	modbus.time_flag = 0;//收发间隔达到标志位清零
}
//作为从机部分内容结束


