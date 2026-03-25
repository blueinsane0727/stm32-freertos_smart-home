#ifndef _DRIVER_W25Q64_H
#define _DRIVER_W25Q64_H

extern SPI_HandleTypeDef hspi1;

typedef enum
{
	Tx_Err,
	Rx_Err,
	Busy,
	error,
	Work_OK
}W25Q64_Return;

#define W25Q64_Timeout 	  500
#define W25Q64_SectorSize 4096
#define W25Q64_NSS_Port   GPIOA
#define W25Q64_NSS_Pin    GPIO_PIN_4

W25Q64_Return W25Q64_Read(uint32_t addr,uint8_t buf,uint32_t len);		//读数据
W25Q64_Return W25Q64_Write(uint32_t addr,uint8_t buf,uint32_t len);		//写数据
W25Q64_Return W25Q64_Erase(uint32_t addr,uint32_t num);					//擦除扇区

#endif
