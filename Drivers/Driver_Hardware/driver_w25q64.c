#include "stm32f1xx_hal.h"
#include "driver_w25q64.h"
#include "FreeRTOS.h"
#include "task.h"

static SPI_HandleTypeDef *Hspi = &hspi1;

/* 函数名称：W25q64_WaitReady
** 输入参数：无
** 返回值：返回标志 W25Q64_Return
**         Work_OK  表示W25Q64正常工作，可以发送数据
**         Tx_Err   表示发送出错
**         Busy     表示W25Q64正在工作展示无法处理其他命令
** 函数功能：查询W25Q64是否处在忙等待状态
*/
static W25Q64_Return W25q64_WaitReady(void)
{
	unsigned char txbuf[] = {0x05,0xff};//读命令
	unsigned char rxbuf[2];
	int timeout = W25Q64_Timeout;
	
	rxbuf[0] = rxbuf[1] = 0;
	while(timeout--)
	{
		if(HAL_OK == HAL_SPI_TransmitReceive(Hspi,txbuf,rxbuf,2,W25Q64_Timeout))
		{
			if((rxbuf[1] & 1) == 0)
			{
				return Work_OK;
			}
			
		}else
		{
			return Tx_Err;
		}
		#ifdef USE_RTOS
			vTaskDelay(1);  
		#else
			HAL_Delay(1);   
		#endif
	}
	return Busy;
}

/* 函数名称：W25Q64_WriteEnable
** 输入参数：无
** 返回值：返回标志 HAL_StatusTypeDef
**         Tx_err  发送错误
**         Work_OK 工作正常
** 函数功能：写使能，发送写使能指令并返回状态标志
*/
static W25Q64_Return W25Q64_WriteEnable(void)
{
	unsigned char txbuf[] = {0x06,0xFF};

	if(HAL_SPI_Transmit(Hspi,txbuf,1,W25Q64_Timeout) != HAL_OK)
	{
		return Tx_Err;
	}
	return Work_OK;
}

/* 函数名称：W25Q64_Read
** 输入参数：uint32_t addr   指定地址
**          uint8_t buf     读到的数据存放在buf里  
**          uint32_t len    要读取的数据长度
** 返回值：返回标志 W25Q64_Return
**         Work_OK  表示W25Q64正常工作
**         Tx_Err   表示发送出错
**         Rx_Err   表示接收出错
** 函数功能：在指定地址读取指定长度的数据
*/
W25Q64_Return W25Q64_Read(uint32_t addr,uint8_t buf,uint32_t len)
{
	unsigned char txbuf[4];
	int err;
	txbuf[0] = 0x03;
	txbuf[1] = (addr << 16) & 0xFF;
	txbuf[2] = (addr << 8) & 0xFF;
	txbuf[3] = (addr << 0) & 0xFF;

	err = HAL_SPI_Transmit(Hspi,txbuf,4,W25Q64_Timeout);
	if(err)
	{
		return Tx_Err;
	}

	err = HAL_SPI_Receive(Hspi,&buf,len,W25Q64_Timeout);
	if(err)
	{
		return Rx_Err;
	}

	return Work_OK;
}

/* 函数名称：W25Q64_Write
** 输入参数：uint32_t addr   指定地址
**          uint8_t buf     要写入的数据 
**          uint32_t len    数据长度
** 返回值：返回标志 W25Q64_Return
**         Work_OK  表示W25Q64正常工作
**         Tx_Err   表示发送出错
**         Busy     等待时间结束还未写完数据
** 函数功能：在指定地址写入指定长度的数据
*/
W25Q64_Return W25Q64_Write(uint32_t addr,uint8_t buf,uint32_t len)
{
	uint8_t txbuf[4];
	uint32_t now_addr = addr;	//目前所在的地址
	W25Q64_Return err;
	uint32_t now_len;			//当前页准备写入的长度
	uint32_t remain_len = len;	//剩余要写入的长度

	now_len = addr & (256-1);	//与255按位与，取低八位，本质是取余计算偏移，即要写入的地址在当前页的第几个
	now_len = 256 - now_len;
	if(now_len >= len)
	{
		now_len = len;
	}

	for(;now_addr <= addr + len;)
	{
		/*写使能 */
		if(W25Q64_WriteEnable() != HAL_OK)
		{
			return Tx_Err;
		}

		txbuf[0] = 0x02;		//页编程指令
		txbuf[1] = (now_addr << 16) & 0xFF;
		txbuf[2] = (now_addr << 8) & 0xFF;
		txbuf[3] = (now_addr << 0) & 0xFF;

		/*发送要写入的地址*/
		if(HAL_SPI_Transmit(Hspi,txbuf,4,W25Q64_Timeout) != HAL_OK)
		{
			return Tx_Err;
		}
		
		/*发送要写的数据*/
		if(HAL_SPI_Transmit(Hspi,&buf,now_len,W25Q64_Timeout) != HAL_OK)
		{
			return Tx_Err;
		}
		
		/*等待数据写入完成*/
		err = W25q64_WaitReady();
		if(err != Work_OK)
		{	
			return err;
		}

		now_addr = now_addr + now_len;		//更新当前地址
		buf = buf + now_len;				//更新buf指针
		remain_len = remain_len - now_len;	//更新剩余长度

		if(remain_len <256)
		{
			now_len = remain_len;
		}else
		{
			now_len = 256;
		}
	}

	return Work_OK;
}

/* 函数名称：W25Q64_Erase
** 输入参数：uint32_t addr   指定地址
**          uint32_t num    指定要擦除的扇区数量
** 返回值：返回标志 W25Q64_Return
**         Work_OK  表示W25Q64正常工作
**         Tx_Err   表示发送出错
**         Busy     等待时间结束还未写完数据
** 函数功能：擦除指定扇区的数据
*/
W25Q64_Return W25Q64_Erase(uint32_t addr,uint32_t num)
{
	uint32_t now_addr = addr;
	unsigned char txbuf[4];
	W25Q64_Return err;

	/*参数校验：最小擦除单位是扇区*/
	if(addr & (W25Q64_SectorSize - 1))
	{
		return error;
	}

	for(uint32_t i = 0;i < num;i++)
	{
		err = W25Q64_WriteEnable();
		if(err != Work_OK)
		{
			return err;
		}

		txbuf[0] = 0x20;
		txbuf[1] = (now_addr >> 16) & 0xFF;
		txbuf[2] = (now_addr >> 8) & 0xFF;
		txbuf[3] = (now_addr >> 0) & 0xFF;

		if(HAL_SPI_Transmit(Hspi,txbuf,4,W25Q64_Timeout) != HAL_OK)
		{
			return Tx_Err;
		}

		now_addr += W25Q64_SectorSize;

		err = W25q64_WaitReady();
		if(err != Work_OK)
		{
			return err;
		}
		
	}

	return Work_OK;
}
