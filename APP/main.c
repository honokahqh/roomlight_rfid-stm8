/*
*********************************************************************
* Copyright	(C), 2012-2013, CS&S. Co., Ltd.
* filename:	main.c
* filepath:	f:\draven\ÖÐ¿Ø°å\zk_dravenv100\app
* --------------------------------------------------------
* History:
* <author>	@ Draven
* <time>    2017/07/19
* <version> V-0.0.1
* <desc>	-build this moudle
*********************************************************************
*/
#include "app.h"
#include "modbusSlaver.h"

/*
*****************************************************
* Method:       main
* Description:
* Author:       @Draven
* Date:  		2017/07/20
* Returns:      void
* Parameter:   	void
* History:
*****************************************************
*/
void main(void)
{
    bsp_driver_init();
    MBS_Init();
    enableInterrupts();
    app_init();
    system_run();
}

void assert_failed(u8 *file, u32 line)
{
    /* User can add his own implementation to report the file name and line number,
       ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
    /* Infinite loop */
    // while (1)
    //{
    // }
}

/*
*********************************************************************
* -main.c	<File end.>
* -f:\draven\ÖÐ¿Ø°å\zk_dravenv100\app
*********************************************************************
*/
