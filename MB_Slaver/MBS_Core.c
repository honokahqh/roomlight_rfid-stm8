/*
*********************************************************************
* Copyright	(C), 2012-2013, CS&S. Co., Ltd.
* filename:	MBS_Core.c
* filepath:	f:\draven\��̬��\modbus_slaverproj\draven-stm8-proj\mb-slaver
* --------------------------------------------------------
* History:
* <author>	@ Draven
* <time>    2017/05/08
* <version> V-0.0.1
* <desc>	-build this moudle
*********************************************************************
*/

#include "modbusSlaver.h"

/*
*****************************************************
* Method:       MBS_CorePoll
* Description:	�������ݰ�. ��ѵ����
* Author:       @Draven
* Date:  		2017/05/08
* Returns:      void
* Parameter:   	void
* History:
*****************************************************
*/
uint8_t mbs_hasNewFrame;
void MBS_CorePoll(void)
{	
	if (eMBS_Port._rxState == 0)	/*δ���յ����� ��ֱ�ӷ���*/
	{	
		return;				
	}

	eMBS_Port._rxState = 0;	 		/* ���־ */

	if (eMBS_Port._rxLen < 4)		/* ���յ�������С��4���ֽھ���Ϊ���� */
	{
		goto err_ret;
	}

	/* վ��ַ (1�ֽڣ� */
	if (eMBS_Port._rxBuff[MBS_FRAME_SLAVER_ADDR] != MBS_SelfAddr)	/* �ж��������͵������ַ�Ƿ���� (0��ַ�㲥) */
	{
		goto err_ret;
	}

	/* ����CRCУ��� */
	if (CRC16_Modbus(eMBS_Port._rxBuff,eMBS_Port._rxLen))
	{
		goto err_ret;
	}
	mbs_hasNewFrame = 1;
	/* ����Ӧ�ò�Э�� */
	MBS_CoreAnalyze();

err_ret:
	eMBS_Port._rxLen = 0;			/*��λ��������ָ��*/
}




/*
*****************************************************
* Method:       MBS_CoreAnalyze
* Description:	��������֡
* Author:       @Draven
* Date:  		2017/05/08
* Returns:      void
* Parameter:   	void
* History:
*****************************************************
*/
void MBS_CoreAnalyze(void)
{
	switch (eMBS_Port._rxBuff[1])		/* ��2���ֽ� ������ */
	{
#if MBS_FUNCTION_01_ENABLE
	case 0x01:
		MBS_Function01H();
		break;
#endif
#if MBS_FUNCTION_02_ENABLE
	case 0x02:
		MBS_Function02H();
		break;
#endif
#if MBS_FUNCTION_03_ENABLE
	case 0x03:
		MBS_Function03H();
		break;
#endif
#if MBS_FUNCTION_04_ENABLE
	case 0x04:
		MBS_Function04H();
		break;
#endif
#if MBS_FUNCTION_05_ENABLE
	case 0x05:
		MBS_Function05H();
		break;
#endif
#if MBS_FUNCTION_06_ENABLE
	case 0x06:
		MBS_Function06H();
		break;
#endif
#if MBS_FUNCTION_0F_ENABLE
	case 0x0F:
		MBS_Function0FH();
		break;
#endif
#if MBS_FUNCTION_10_ENABLE
	case 0x10:
		MBS_Function10H();
		break;
#endif
	default:
		MBS_PortSendAck(MBS_EX_ILLEGAL_FUNCTION);
		break;
	}
}


/*
*****************************************************
* Method:       MBS_Init
* Description:	Э��ջ��ʼ��
* Author:       @Draven 
* Date:  		2017/05/08
* Returns:      void
* Parameter:   	void
* History:
*****************************************************
*/
void MBS_Init(void)
{
	MBS_UserValueMapping();
}

/*
*********************************************************************
* -MBS_Core.c	<File end.>
* -f:\draven\��̬��\modbus_slaverproj\draven-stm8-proj\mb-slaver
*********************************************************************
*/


