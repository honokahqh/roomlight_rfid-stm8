/*
*********************************************************************
* Copyright	(C), 2012-2013, CS&S. Co., Ltd.
* filename:	StateOS.h
* filepath:	f:\draven\�пذ�\zk_dravenv100\stateos
* --------------------------------------------------------
* History:
* <author>	@ Draven
* <time>    2017/07/20
* <version> V-0.0.1
* <desc>	-build this moudle
*********************************************************************
*/
#ifndef _STATEOS_H
#define _STATEOS_H

#define MAXTASKS 2U
volatile static unsigned char timers[MAXTASKS];

/***************************************************************/
/* ����ʼ */
#define _SS	                                            \
	static unsigned char _lc = 0U;                      \
	switch (_lc)                                        \
{                                                       \
	case 0U:
/***************************************************************/
/* ������� */
#define _EE	                                            \
	break;                                              \
	default:                                            \
	break;                                              \
}										                \
	_lc = 0U;                                           \
	return 255U;
/***************************************************************/
/* �������ȴ� */
#define WaitX(tickets)                                  \
	_lc = (unsigned char)(__LINE__) % 255U + 1U;		\
if (_lc)                                                \
{                                                       \
	return (tickets);                                   \
}                                                       \
	break;                                              \
case ((unsigned char)(__LINE__) % 255U) + 1U:
/***************************************************************/
/* �������� */
#define RunTask(TaskName,TaskID)	                    \
do{                                                     \
	if (timers[(TaskID)] == 0U)                         \
	{                                                   \
		timers[(TaskID)] = (TaskName)();                \
	}                                                   \
} while (0)
/***************************************************************/
/* ������������ �ڲ����� WaitX*/
#define RunTaskA(TaskName,TaskID)	                    \
{                                                       \
	if (timers[(TaskID)] == 0U)                         \
	{                                                   \
		timers[(TaskID)] = (TaskName)();                \
		continue;                                       \
	}                                                   \
}    /*ǰ����������ȱ�ִ֤��*/
/***************************************************************/
/* ����һ���Ӻ��� */
#define CallSub(SubTaskName)                            \
do {                                                    \
	unsigned char currdt;                               \
	_lc=(unsigned char)(__LINE__)%255U+1U;              \
	if(_lc)                                             \
	{                                                   \
		return 0U;                                      \
	}                                                   \
		break;                                          \
	case (unsigned char)(__LINE__)%255U+1U:             \
		currdt=(SubTaskName)();                         \
	if(currdt!=255U)                                    \
		{                                               \
			return currdt;                              \
		}                                               \
} while(0);
/***************************************************************/
/* ��ʼ������ʵ�����Ǹ� timers ���� */
#define InitTasks()	                                    \
do {                                                    \
		unsigned char i;                                \
		for(i=MAXTASKS;i>0U ;i--)                       \
		{                                               \
			timers[i-1U]=0U;                            \
		}                                               \
} while(0)
/***************************************************************/
/* �������� */
#define UpdateTimers()	                                \
do {                                                    \
	unsigned char i;                                    \
	for (i = MAXTASKS; i > 0U; i--)                     \
	{                                                   \
		if (timers[i - 1U] != 0U)                       \
		{                                               \
			if (timers[i - 1U] != 255U)                 \
			{                                           \
				timers[i - 1U]--;                       \
			}                                           \
		}                                               \
	}                                                   \
} while (0)
/***************************************************************/
#define SEM				unsigned int 
/***************************************************************/
	/*��ʼ���ź���*/
#define InitSem(sem)		(sem)=0;
/***************************************************************/
	/*�ȴ��ź���*/
#define WaitSem(sem)		do{ (sem)=1; WaitX(0); if ((sem)>0) return 1;} while(0);
/***************************************************************/
	/*�ȴ��ź�����ʱ������� ��ʱ��tickets ���Ϊ0xFFFE*/
#define WaitSemX(sem,tickets)  do { (sem)=(tickets)+1; WaitX(0); if((sem)>1){ (sem)--;  return 1;} } while(0);
/***************************************************************/
	/*�����ź���*/
#define SendSem(sem)		do {(sem)=0;} while(0);





#endif
	/*
	*********************************************************************
	* -StateOS.h	<File end.>
	* -f:\draven\�пذ�\zk_dravenv100\stateos
	*********************************************************************
	*/

