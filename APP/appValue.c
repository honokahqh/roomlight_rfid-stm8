/*
*********************************************************************
* Copyright	(C), 2012-2013, CS&S. Co., Ltd.
* filename:	appValue.c
* filepath:	f:\draven\�пذ�\zk_dravenv100\app 
* --------------------------------------------------------
* History:
* <author>	@ Draven
* <time>    2017/07/21
* <version> V-0.0.1   
* <desc>	-build this moudle		����ϵͳȫ����Ҫ����
*********************************************************************
*/

#include "modbusSlaver.h"



uint8_t coilState[USER_COIL_NUM];				/* ��Ȧ״̬ */
uint8_t coilStateLast[USER_COIL_NUM];

uint16_t holdRegValue[USER_HOLDREG_NUM];		/* ���ּĴ���״̬ */
uint16_t holdRegValueLast[USER_HOLDREG_NUM];		

uint8_t	MBS_SelfAddr = MBS_Addr;			/* ������ַ */


/* 
*********************************************************************
* -appValue.c	<File end.>
* -f:\draven\�пذ�\zk_dravenv100\app
*********************************************************************
*/

