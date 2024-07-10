/*
*********************************************************************
* Copyright	(C), 2012-2013, CS&S. Co., Ltd.
* filename:	app.c
* filepath:	f:\draven\中控板\zk_dravenv100\app

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

static int modbus_sevice(struct pt *pt); /* MODBUS 通信服务 */
static int rgb_sevice(struct pt *pt);	 /* RGB 灯带服务 */
static int in_operation(struct pt *pt);	 /* 系统运行监控 看门狗喂食 数据保存*/
static int rfid_sevice(struct pt *pt);	 /* rfid解码服务 */
static int bell_sevice(struct pt *pt);	 /* 蜂鸣器服务 */

#define Dev_version 115 /* @2017/08/15 09:56:57 上电校验数 也等同于版本号 */

/*
*****************************************************
* Method:       app_init
* Description:	应用程序初始化
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
	/* 检查是否第一次上电 eeeaddr 100 */
	temp[0] = bsp_eepromReadOneByte(0);
	if (temp[0] != Dev_version) /* 第一次上电 */
	{
		/* @2017/08/15 10:03:49 开关和模式 rfid卡*/
		coilState[COIL_SWITCH] = coilStateLast[COIL_SWITCH] = 1;
		coilState[COIL_MODE] = coilStateLast[COIL_MODE] = 0;
		bsp_eepromWriteBuff(1, 2, coilState);
		/* @2017/08/15 09:37:57 再保存保持寄存器内容 */
		holdRegValue[HREG_DELAY_TIME] = holdRegValueLast[HREG_DELAY_TIME] = 50;
		holdRegValue[HREG_START_ADDR] = holdRegValueLast[HREG_START_ADDR] = 0;
		holdRegValue[HREG_END_ADDR] = holdRegValueLast[HREG_END_ADDR] = 1;
		holdRegValue[HREG_COLOR_HOLD] = holdRegValueLast[HREG_COLOR_HOLD] = 0xffff;
		bsp_eepromWriteBuff(20, 8, (uint8_t *)&holdRegValue[HREG_DELAY_TIME]);
		/* @2017/08/15 09:46:27 初始化颜色表 */
		holdRegValue[HREG_TABLE_COLOR1] = holdRegValueLast[HREG_TABLE_COLOR1] = 0xF800; // 红色
		holdRegValue[HREG_TABLE_COLOR2] = holdRegValueLast[HREG_TABLE_COLOR2] = 0x001f; // 蓝色
		bsp_eepromWriteBuff(28, 32, (uint8_t *)&holdRegValue[HREG_TABLE_COLOR1]);
		/* @2017/08/15 09:58:23 写入第一次上电标志 */
		bsp_eepromWriteOneByte(0, Dev_version);
	}
	else /* 不是第一次上电 */
	{
		/* @2017/08/15 10:03:49 开关和模式 rfid卡*/
		bsp_eepromReadBuff(1, 2, temp);
		coilState[COIL_SWITCH] = coilStateLast[COIL_SWITCH] = temp[0];
		coilState[COIL_MODE] = coilStateLast[COIL_MODE] = temp[1];
		/* @2017/08/15 10:04:06 延时索引和颜色 */
		bsp_eepromReadBuff(20, 8, temp);
		holdRegValue[HREG_DELAY_TIME] = holdRegValueLast[HREG_DELAY_TIME] = *((uint16_t *)&temp[0]);
		holdRegValue[HREG_START_ADDR] = holdRegValueLast[HREG_START_ADDR] = *((uint16_t *)&temp[2]);
		holdRegValue[HREG_END_ADDR] = holdRegValueLast[HREG_END_ADDR] = *((uint16_t *)&temp[4]);
		holdRegValue[HREG_COLOR_HOLD] = holdRegValueLast[HREG_COLOR_HOLD] = *((uint16_t *)&temp[6]);
		/* @2017/08/15 10:06:37 颜色表 */
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
* Description:	状态机跑起来
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
* Description:	MODBUS 通信服务
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
		MBS_PhysicalReserveBuff(uart_state.rx_buff, uart_state.rx_len); /* 发送数据帧 到解析 */
		uart_state.has_data = 0;
		uart_state.rx_len = 0;
		MBS_CorePoll(); /* 解析通信数据 */
	}
	PT_END(pt);
}
/*
*****************************************************
* Method:       rfid_sevice
* Description:	rfid解码服务
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
		// 	// debug用,单卡一直刷进while就是有问题
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
* Description:	蜂鸣器服务
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
				GPIOD->ODR &= ~GPIO_PIN_2; // 开蜂鸣器
				ssDelayA = sysTime + holdRegValue[HREG_BELL_DELAY];
				PT_WAIT_UNTIL(pt, sysTime >= ssDelayA);
				GPIOD->ODR |= GPIO_PIN_2; // 关蜂鸣器
				ssDelayA = sysTime + holdRegValue[HREG_BELL_DELAY];
				PT_WAIT_UNTIL(pt, sysTime >= ssDelayA);
			}
		}

		if (!init_flag)
		{
			init_flag = 1;
			GPIOD->ODR &= ~GPIO_PIN_2; // 开蜂鸣器
			ssDelayA = sysTime + 200;
			PT_WAIT_UNTIL(pt, sysTime >= ssDelayA);
			GPIOD->ODR |= GPIO_PIN_2; // 关蜂鸣器
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
* Description:	RGB 灯带服务
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
		if (coilState[COIL_SWITCH]) /* 总开关 */
		{
			if (coilState[COIL_MODE]) /* 模式 */
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
* Description:	系统运行监控
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
		eeprom_sevice();  /* 数据有改变就会被保存 */
		bsp_dog_reload(); /* 定时喂狗 */
		if (mbs_hasNewFrame)
		{
			mbsTimeout = 0;
		}
		mbsTimeout++;
		if (mbsTimeout > 600) // 超时reboot
			WWDG->CR = 0x80;
	}
	PT_END(pt);
}

/*
*********************************************************************
* -app.c	<File end.>
* -f:\draven\中控板\zk_dravenv100\app
*********************************************************************
*/
