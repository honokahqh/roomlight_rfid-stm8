/*
*********************************************************************
* Copyright	(C), 2012-2013, CS&S. Co., Ltd.
* filename:	MBS_Physical.c
* filepath:	f:\draven\��̬��\modbus_slaverproj\draven-stm8-proj\mb-slaver
* --------------------------------------------------------
* History:
* <author>	@ Draven
* <time>    2017/05/08
* <version> V-0.0.1
* <desc>	-��֧�ֶ�˿ڹ���Э��ջ��ֻ��Ҫ�޸Ĵ��ļ�
*********************************************************************
*/

#include "modbusSlaver.h"

/*
*****************************************************
* Method:       MBS_PhysicalSendBuff
* Description:	����˿ڷ���һ֡���ݣ��˺�������Ҫ�޸ġ�������ModbusPort�ļ�����
* Author:       @Draven 
* Date:  		2017/05/08
* Returns:      void
* Parameter:   	uint8_t * _str
* Parameter:   	uint8_t _strLen
* History:
*****************************************************
*/
void MBS_PhysicalSendBuff(uint8_t *_str, uint8_t _strLen)
{
	mbs_sendData(_str, _strLen);
}

/*
*****************************************************
* Method:       MBS_PhysicalReserveBuff
* Description:	��Ӳ�����յ�һ֡���ݺ󽫻���ô˺��������յ�������֡����Э��ջ������н���
* Author:       @Draven 
* Date:  		2017/05/08
* Returns:      void
* Parameter:   	uint8_t * _str
* Parameter:   	uint8_t _strLen
* History:
*****************************************************
*/
void MBS_PhysicalReserveBuff(uint8_t *_str, uint8_t _strLen)
{
	uint16_t	i;
	for ( i = 0; i < _strLen; i++)
	{
		eMBS_Port._rxBuff[i] = _str[i];
	}
	eMBS_Port._rxLen = _strLen;
	eMBS_Port._rxState = 1;
}

/*
*********************************************************************
* -MBS_Physical.c	<File end.>
* -f:\draven\��̬��\modbus_slaverproj\draven-stm8-proj\mb-slaver
*********************************************************************
*/



