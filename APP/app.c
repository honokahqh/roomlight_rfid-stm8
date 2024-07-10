/*
*********************************************************************
* Copyright	(C), 2012-2013, CS&S. Co., Ltd.
* filename:	app.c
* filepath:	f:\draven\�пذ�\zk_dravenv100\app

* --------------------------------------------------------
* History:
* <author>	@ Draven
* <time>    2017/07/19
* <version> V-0.0.1
* <desc>	-build this moudle
*********************************************************************
*/

#include "pt.h"
#include "modbusSlaver.h"
#include "upData.h"
#include "bsp_rgb.h"
static struct pt pt_modbus;
static struct pt pt_rgb;
static struct pt pt_monitor;
static struct pt pt_rfid;
static struct pt pt_bell;

static int modbus_sevice(struct pt *pt); /* MODBUS ͨ�ŷ��� */
static int rgb_sevice(struct pt *pt);	 /* RGB �ƴ����� */
static int in_operation(struct pt *pt);	 /* ϵͳ���м�� ���Ź�ιʳ ���ݱ���*/
static int rfid_sevice(struct pt *pt);	 /* rfid������� */
static int bell_sevice(struct pt *pt);	 /* ���������� */

#define Dev_version 115 /* @2017/08/15 09:56:57 �ϵ�У���� Ҳ��ͬ�ڰ汾�� */

/*
*****************************************************
* Method:       app_init
* Description:	Ӧ�ó����ʼ��
* Author:       @Draven
* Date:  		2017/07/20
* Returns:      void
* Parameter:   	void
* History:
*****************************************************
*/
void app_init(void)
{
	uint8_t temp[32] = {0};
	bsp_rgb_load565(0xFFFF);
	/* ����Ƿ��һ���ϵ� eeeaddr 100 */
	temp[0] = bsp_eepromReadOneByte(0);
	if (temp[0] != Dev_version) /* ��һ���ϵ� */
	{
		/* @2017/08/15 10:03:49 ���غ�ģʽ rfid��*/
		coilState[COIL_SWITCH] = coilStateLast[COIL_SWITCH] = 1;
		coilState[COIL_MODE] = coilStateLast[COIL_MODE] = 0;
		bsp_eepromWriteBuff(1, 2, coilState);
		/* @2017/08/15 09:37:57 �ٱ��汣�ּĴ������� */
		holdRegValue[HREG_DELAY_TIME] = holdRegValueLast[HREG_DELAY_TIME] = 50;
		holdRegValue[HREG_START_ADDR] = holdRegValueLast[HREG_START_ADDR] = 0;
		holdRegValue[HREG_END_ADDR] = holdRegValueLast[HREG_END_ADDR] = 1;
		holdRegValue[HREG_COLOR_HOLD] = holdRegValueLast[HREG_COLOR_HOLD] = 0xffff;
		bsp_eepromWriteBuff(20, 8, (uint8_t *)&holdRegValue[HREG_DELAY_TIME]);
		/* @2017/08/15 09:46:27 ��ʼ����ɫ�� */
		holdRegValue[HREG_TABLE_COLOR1] = holdRegValueLast[HREG_TABLE_COLOR1] = 0xF800; // ��ɫ
		holdRegValue[HREG_TABLE_COLOR2] = holdRegValueLast[HREG_TABLE_COLOR2] = 0x001f; // ��ɫ
		bsp_eepromWriteBuff(28, 32, (uint8_t *)&holdRegValue[HREG_TABLE_COLOR1]);
		/* @2017/08/15 09:58:23 д���һ���ϵ��־ */
		bsp_eepromWriteOneByte(0, Dev_version);
	}
	else /* ���ǵ�һ���ϵ� */
	{
		/* @2017/08/15 10:03:49 ���غ�ģʽ rfid��*/
		bsp_eepromReadBuff(1, 2, temp);
		coilState[COIL_SWITCH] = coilStateLast[COIL_SWITCH] = temp[0];
		coilState[COIL_MODE] = coilStateLast[COIL_MODE] = temp[1];
		/* @2017/08/15 10:04:06 ��ʱ��������ɫ */
		bsp_eepromReadBuff(20, 8, temp);
		holdRegValue[HREG_DELAY_TIME] = holdRegValueLast[HREG_DELAY_TIME] = *((uint16_t *)&temp[0]);
		holdRegValue[HREG_START_ADDR] = holdRegValueLast[HREG_START_ADDR] = *((uint16_t *)&temp[2]);
		holdRegValue[HREG_END_ADDR] = holdRegValueLast[HREG_END_ADDR] = *((uint16_t *)&temp[4]);
		holdRegValue[HREG_COLOR_HOLD] = holdRegValueLast[HREG_COLOR_HOLD] = *((uint16_t *)&temp[6]);
		/* @2017/08/15 10:06:37 ��ɫ�� */
		bsp_eepromReadBuff(28, 32, temp);
		for (uint8_t i = 0; i < 16; i++)
		{
			holdRegValue[HREG_TABLE_COLOR1 + i] = holdRegValueLast[HREG_TABLE_COLOR1 + i] = *((uint16_t *)&temp[i * 2]);
		}
	}
	holdRegValue[HREG_MY_ADDR] = holdRegValueLast[HREG_MY_ADDR] = MBS_Addr;
	holdRegValue[HREG_MY_VERSION] = holdRegValueLast[HREG_MY_VERSION] = Dev_version;

	delay_ms(MBS_Addr * 2 + 100);
	temp[0] = 0xff;
	temp[1] = MBS_Addr;
	temp[2] = 0xfe;
	mbs_sendData(temp, 3);
}

/*
*****************************************************
* Method:       system_run
* Description:	״̬��������
* Author:       @Draven
* Date:  		2017/07/20
* Returns:      void
* Parameter:   	void
* History:
*****************************************************
*/
void system_run(void)
{
	/* Initialize the protothread state variables with PT_INIT(). */
	PT_INIT(&pt_modbus);
	PT_INIT(&pt_rgb);
	PT_INIT(&pt_monitor);
	PT_INIT(&pt_rfid);
	PT_INIT(&pt_bell);

	while (1)
	{
		modbus_sevice(&pt_modbus);
		rgb_sevice(&pt_rgb);
		rfid_sevice(&pt_rfid);
		bell_sevice(&pt_bell);
		in_operation(&pt_monitor);
	}
}

/*
*****************************************************
* Method:       modbus_sevice
* Description:	MODBUS ͨ�ŷ���
* Author:       @Draven
* Date:  		2017/07/21
* Returns:      int
* Parameter:   	struct pt * pt
* History:
*****************************************************
*/
static int modbus_sevice(struct pt *pt)
{
	PT_BEGIN(pt);

	while (1)
	{
		PT_WAIT_UNTIL(pt, uart_state.has_data);
		upData_CheckFrame(uart_state.rx_buff, uart_state.rx_len);
		MBS_PhysicalReserveBuff(uart_state.rx_buff, uart_state.rx_len); /* ��������֡ ������ */
		uart_state.has_data = 0;
		uart_state.rx_len = 0;
		MBS_CorePoll(); /* ����ͨ������ */
	}
	PT_END(pt);
}
/*
*****************************************************
* Method:       rfid_sevice
* Description:	rfid�������
* Author:       @linkai
* Date:  		2018/07/03
* Returns:      int
* Parameter:   	struct pt * pt
* History:
*****************************************************
*/
uint8_t init_flag = 0;
static int rfid_sevice(struct pt *pt)
{
	static uint32_t sys_ms_last;
	PT_BEGIN(pt);
	while (1)
	{
		PT_WAIT_UNTIL(pt, ID_Card_Info.has_card);
		ID_Card_Info.has_card = 0;
		// if ((holdRegValue[HREG_RFID_ID1] != ID_Card_Info.ID[1]) ||
		// 	(holdRegValue[HREG_RFID_ID2] != ID_Card_Info.ID[2]) ||
		// 	(holdRegValue[HREG_RFID_ID3] != ID_Card_Info.ID[3]) ||
		// 	(holdRegValue[HREG_RFID_ID4] != ID_Card_Info.ID[4]))
		// {
		// 	// debug��,����һֱˢ��while����������
		// 	if (init_flag == 0)
		// 		while (1)
		// 			;
		// }
		if (sysTime > sys_ms_last + 600 ||
			(holdRegValue[HREG_RFID_ID1] != ID_Card_Info.ID[1]) ||
			(holdRegValue[HREG_RFID_ID2] != ID_Card_Info.ID[2]) ||
			(holdRegValue[HREG_RFID_ID3] != ID_Card_Info.ID[3]) ||
			(holdRegValue[HREG_RFID_ID4] != ID_Card_Info.ID[4]))
		{
			coilState[COIL_CARD] = 1;
			holdRegValue[HREG_RFID_ID1] = ID_Card_Info.ID[1];
			holdRegValue[HREG_RFID_ID2] = ID_Card_Info.ID[2];
			holdRegValue[HREG_RFID_ID3] = ID_Card_Info.ID[3];
			holdRegValue[HREG_RFID_ID4] = ID_Card_Info.ID[4];
			init_flag = 0;
		}
		sys_ms_last = sysTime;
	}
	PT_END(pt);
}
/*
*****************************************************
* Method:       bell_sevice
* Description:	����������
* Author:       @linkai
* Date:  		2018/07/03
* Returns:      int
* Parameter:   	struct pt * pt
* History:
*****************************************************
*/
static int bell_sevice(struct pt *pt)
{
	static uint32_t count, ssDelayA = 0;
	PT_BEGIN(pt);
	while (1)
	{
		if (coilState[COIL_BELL])
		{
			coilState[COIL_BELL] = 0;
			for (count = 0; count < holdRegValue[HREG_BELL_NUM]; count++)
			{
				GPIOD->ODR &= ~GPIO_PIN_2; // ��������
				ssDelayA = sysTime + holdRegValue[HREG_BELL_DELAY];
				PT_WAIT_UNTIL(pt, sysTime >= ssDelayA);
				GPIOD->ODR |= GPIO_PIN_2; // �ط�����
				ssDelayA = sysTime + holdRegValue[HREG_BELL_DELAY];
				PT_WAIT_UNTIL(pt, sysTime >= ssDelayA);
			}
		}

		if (!init_flag)
		{
			init_flag = 1;
			GPIOD->ODR &= ~GPIO_PIN_2; // ��������
			ssDelayA = sysTime + 200;
			PT_WAIT_UNTIL(pt, sysTime >= ssDelayA);
			GPIOD->ODR |= GPIO_PIN_2; // �ط�����
			ssDelayA = sysTime + 200;
			PT_WAIT_UNTIL(pt, sysTime >= ssDelayA);
		}
		ssDelayA = sysTime + 500;
		PT_WAIT_UNTIL(pt, sysTime >= ssDelayA);
	}
	PT_END(pt);
}
/*
*****************************************************
* Method:       rgb_sevice
* Description:	RGB �ƴ�����
* Author:       @Draven
* Date:  		2017/07/22
* Returns:      int
* Parameter:   	struct pt * pt
* History:
*****************************************************
*/
static int rgb_sevice(struct pt *pt)
{
	static uint32_t ssDelay = 0;
	static uint8_t rgbTimCount = 0;
	static uint8_t rgbIndex = 0;

	PT_BEGIN(pt);
	rgbIndex = holdRegValue[HREG_START_ADDR];
	while (1)
	{
		ssDelay = sysTime + 10;
		PT_WAIT_UNTIL(pt, sysTime >= ssDelay);
		if (coilState[COIL_SWITCH]) /* �ܿ��� */
		{
			if (coilState[COIL_MODE]) /* ģʽ */
			{
				bsp_rgb_load565(holdRegValue[HREG_COLOR_HOLD]);
			}
			else
			{
				rgbTimCount++;
				if (rgbTimCount > holdRegValue[HREG_DELAY_TIME])
				{
					rgbTimCount = 0;
					if ((rgbIndex < holdRegValue[HREG_START_ADDR]) || (rgbIndex > holdRegValue[HREG_END_ADDR]))
					{
						rgbIndex = holdRegValue[HREG_START_ADDR];
					}
					bsp_rgb_load565(holdRegValue[HREG_TABLE_COLOR1 + rgbIndex]);
					rgbIndex++;
				}
			}
		}
		else
		{
			bsp_rgb_load565(0);
		}
	}
	PT_END(pt);
}

/*
*****************************************************
* Method:       in_operation
* Description:	ϵͳ���м��
* Author:       @Draven
* Date:  		2017/07/22
* Returns:      int
* Parameter:   	struct pt * pt
* History:
*****************************************************
*/
extern uint8_t mbs_hasNewFrame;
uint32_t mbsTimeout = 0;
static int in_operation(struct pt *pt)
{
	static uint32_t ssDelay = 0;
	PT_BEGIN(pt);
	while (1)
	{
		ssDelay = sysTime + 100;
		PT_WAIT_UNTIL(pt, sysTime >= ssDelay);
		eeprom_sevice();  /* �����иı�ͻᱻ���� */
		bsp_dog_reload(); /* ��ʱι�� */
		if (mbs_hasNewFrame)
		{
			mbsTimeout = 0;
		}
		mbsTimeout++;
		if (mbsTimeout > 600) // ��ʱreboot
			WWDG->CR = 0x80;
	}
	PT_END(pt);
}

/*
*********************************************************************
* -app.c	<File end.>
* -f:\draven\�пذ�\zk_dravenv100\app
*********************************************************************
*/
