#include "XM7903.h"
u8 Read_Noise_Data[8] = {0x01,0x03,0x00,0x00,0x00,0x01,0x84,0x0A};    
void XM7903_SendPacket(void)
{
    Serial_SendArray(USART3,Read_Noise_Data,TxPaket_Length);//发送数据包     
}
//XM7903_SendPacket(USART3,Read_Noise_Data,TxPaket_Length);
float Get_Nosie_Data(void){
	u16 Noise = 0;
	u16 crc = (Usart3_RxPaket[5] << 8) | Usart3_RxPaket[6];
	u16 calcCrc = crc16(Usart3_RxPaket, 5);  // 计算前N-2字节的CRC	 
	  if (crc == calcCrc){
		Noise = (Usart3_RxPaket[3] << 8) | Usart3_RxPaket[4];  
	}	
 	return (float)Noise / 10.0;      
}






