// bsp msg
#include "bsp_rgb.h"

static void bsp_rfid_init(void);
static void bsp_rgb_init(void);
static void bsp_rs485_init(void);
static void bsp_dog_init(void);
static void bsp_clock_init(void);

volatile uint32_t sysTime = 0, sys_50us_id = 0; /* 周期时间 49天*/
uart_state_t uart_state;

void bsp_driver_init(void)
{
	uint8_t *p;

	/* Clear High speed internal clock prescaler */
	CLK->CKDIVR &= (uint8_t)(~CLK_CKDIVR_HSIDIV);
	/* Set High speed internal clock prescaler */
	CLK->CKDIVR |= (uint8_t)CLK_PRESCALER_HSIDIV1;
	CLK->ICKR |= 0x80; /* 开启内部低速时钟 看门狗 */

	p = (uint8_t *)0x4803;

	if (*p != 0x01) /* @2017/08/15 11:17:06 IO 重映射 */
	{
		do
		{
			FLASH->DUKR = FLASH_RASS_KEY2;
			FLASH->DUKR = FLASH_RASS_KEY1;
		} while (!(FLASH->IAPSR & 0x08));
		FLASH->CR2 |= 0x80; /* @2017/08/15 11:16:13 选项字节操作使能 */
		FLASH->NCR2 &= 0x7F;
		*p++ = 0x01; /* @2017/08/15 11:01:19 OPT2 */
		while (!(FLASH->IAPSR & 0x04))
			;
		*p = 0xFE; /* @2017/08/15 11:01:26 NOPT2 */
		while (!(FLASH->IAPSR & 0x04))
			;
		FLASH->IAPSR &= 0xF7;
		FLASH->CR2 &= 0x7F;
		FLASH->NCR2 |= 0x80;
	}
	bsp_clock_init();
	bsp_rgb_init();
	bsp_rfid_init();
	bsp_rs485_init();
	GPIOD->DDR |= GPIO_PIN_2; /* 输出模式 */
	GPIOD->CR1 |= GPIO_PIN_2; /* 推挽输出 */
	GPIOD->ODR |= GPIO_PIN_2; // 关蜂鸣器
	bsp_dog_init();
}

void bsp_clock_init(void)
{
	/* Set the Prescaler value */
	TIM4->PSCR = (uint8_t)(TIM4_PRESCALER_8);
	/* Set the Autoreload value */
	TIM4->ARR = 99;
	/* Clear the flags (rc_w0) clear this bit by writing 0. Writing ??has no effect*/
	TIM4->SR1 = (uint8_t)(~TIM4_FLAG_UPDATE);
	/* Enable the Interrupt sources */
	TIM4->IER |= (uint8_t)TIM4_IT_UPDATE;
	/* set or Reset the CEN Bit */
	TIM4->CR1 |= TIM4_CR1_CEN;
}

INTERRUPT_HANDLER(TIM4_UPD_OVF_IRQHandler, 23)
{
	static uint8_t count_50us = 0;
	TIM4->SR1 = (uint8_t)(~TIM4_IT_UPDATE);
	sys_50us_id++;
	count_50us++;
	if (count_50us >= 20)
	{
		count_50us = 0;
		sysTime++;
		if (uart_state.IDLE)
		{
			uart_state.IDLE++;
			if (uart_state.IDLE > UART_IDLE_TIMEOUT)
			{
				uart_state.IDLE = 0;
				uart_state.has_data = 1;
			}
		}
	}
}

void delay_ms(uint16_t ms)
{
	uint32_t temp = sysTime;
	while ((sysTime - temp) < ms)
		;
}

void bsp_dog_init(void)
{
	IWDG->KR = 0xCC;
	IWDG->KR = 0x55;

	IWDG->PR = 0x05;
	IWDG->RLR = 0xff;

	IWDG->KR = 0x00;
	IWDG->KR = 0xAA;
}

void bsp_dog_reload(void)
{
	IWDG->KR = 0xAA;
}

static void bsp_rs485_init(void)
{
	/* 使能端 */
	GPIOA->DDR |= GPIO_PIN_2; /* 输出模式 */
	GPIOA->CR1 |= GPIO_PIN_2; /* 推挽输出 */
	GPIOA->CR2 |= GPIO_PIN_2; /* 输出速度最大为 10MHZ */
	_RS485_RX();
	/* TXD */
	GPIOD->DDR |= GPIO_PIN_5; /* 输出模式 */
	GPIOD->CR1 |= GPIO_PIN_5; /* 推挽输出 */
	GPIOD->CR2 |= GPIO_PIN_5; /* 输出速度最大为 10MHZ */
	/* RXD */
	GPIOD->DDR &= ~GPIO_PIN_6; /* 输入模式 */
	GPIOD->CR1 &= ~GPIO_PIN_6; /* 浮空输入 */
	GPIOD->CR2 &= ~GPIO_PIN_6; /* 禁止外部中断 */
	/***************************************************************/
	CLK->PCKENR1 |= CLK_PCKENR1_UART1;
	UART1->CR1 = 0x00; /* 无校验 1位停止位 串口模式*/
	UART1->CR2 = 0x00;
	UART1->CR3 = 0x00;

	UART1->BRR2 = 0x02; /* 波特率 9600 */
	UART1->BRR1 = 0x68;

	UART1->CR2 = 0x2C; /* 允许接收和发送 ，产生接收中断 */
					   /***************************************************************/
}

INTERRUPT_HANDLER(UART1_RX_IRQHandler, 18)
{
	if (UART1->SR & 0x20)
	{
		if (uart_state.rx_len >= UART_BUF_MAX)
			uart_state.rx_len = 0;
		uart_state.rx_buff[uart_state.rx_len++] = UART1->DR;
		uart_state.IDLE = 1;
	}
}

void mbs_sendData(uint8_t *_str, uint8_t _strLen)
{
	unsigned char i;
	_RS485_TX();
	while ((UART1->SR & 0x80) == 0x00)
		;
	for (i = 0; i < _strLen; i++)
	{
		UART1->DR = _str[i];
		while ((UART1->SR & 0x80) == 0x00)
			;
	}
	delay_ms(2);
	_RS485_RX();
}

static void bsp_rgb_init()
{
	/***************************************************************/
	GPIOC->DDR |= GPIO_PIN_6; /* 输出模式 */
	GPIOC->CR1 |= GPIO_PIN_6; /* 推挽输出 */
	GPIOC->CR2 |= GPIO_PIN_6; /* 输出速度最大为 10MHZ */

	GPIOC->DDR |= GPIO_PIN_3; /* 输出模式 */
	GPIOC->CR1 |= GPIO_PIN_3; /* 推挽输出 */
	GPIOC->CR2 |= GPIO_PIN_3; /* 输出速度最大为 10MHZ */

	GPIOC->DDR |= GPIO_PIN_4; /* 输出模式 */
	GPIOC->CR1 |= GPIO_PIN_4; /* 推挽输出 */
	GPIOC->CR2 |= GPIO_PIN_4; /* 输出速度最大为 10MHZ */
	/***************************************************************/

	TIM1->CR1 &= ~0x01; // 关闭TIM1

	TIM1->PSCRH = 0;
	TIM1->PSCRL = 0; // 16M不分频

	TIM1->ARRH = 0X06;
	TIM1->ARRL = 0X3F; // 10KHz

	TIM1->CR1 |= 0x80; // 使能ARP,边沿对齐，向上计数
	TIM1->EGR |= 0x01; // 更新TIM1，使PSC有效
	TIM1->EGR |= 0x20; // 重新初始化TIM1

	TIM1->CCR1H = 0;
	TIM1->CCR1L = 0;
	TIM1->CCMR1 = 0X68; // 配置TIM1_CH1为PWM1模式输出

	TIM1->CCR3H = 0;
	TIM1->CCR3L = 0;
	TIM1->CCMR3 = 0X68; // 配置TIM1_CH3为PWM1模式输出

	TIM1->CCR4H = 0;
	TIM1->CCR4L = 0;
	TIM1->CCMR4 = 0X68; // 配置TIM1_CH4为PWM1模式输出

	TIM1->CCER1 |= 0x01; // Enable TIM1_CH1 channel
	TIM1->CCER2 |= 0x01; // Enable TIM1_CH3 channel
	TIM1->CCER2 |= 0x10; // Enable TIM1_CH4 channel

	TIM1->BKR |= 0x80;
	TIM1->CR1 |= 0x01; // 使能TIM1}
}

int gamma_corrected_values[32] = {0, 1, 2, 5, 11, 18, 26, 37,
								  50, 65, 82, 102, 123, 147, 173, 202,
								  233, 266, 302, 340, 381, 424, 470, 518,
								  569, 622, 679, 737, 799, 863, 930, 1000};
void bsp_rgb_load565(uint16_t color565)
{
	uint16_t color;

	color = gamma_corrected_values[(color565 & 0x001f)];
	TIM1->CCR3H = color >> 8;
	TIM1->CCR3L = color & 0x00ff;
	TIM1->CCMR3 = 0X68; // 配置TIM1_CH4为PWM1模式输出

	color = gamma_corrected_values[(color565 & 0x07e0) >> 6];
	TIM1->CCR1H = color >> 8;
	TIM1->CCR1L = color & 0x00ff;
	TIM1->CCMR1 = 0X68; // 配置TIM1_CH1为PWM1模式输出

	color = gamma_corrected_values[(color565 & 0xf800) >> 11];
	TIM1->CCR4H = color >> 8;
	TIM1->CCR4L = color & 0x00ff;
	TIM1->CCMR4 = 0X68; // 配置TIM1_CH3为PWM1模式输出
}

volatile ID_Card_t ID_Card;
volatile ID_Card_Info_t ID_Card_Info;
static void bsp_rfid_init()
{
	GPIO_Init(GPIOD, GPIO_PIN_3, GPIO_MODE_IN_PU_IT);								   // PD4 输入上拉 有中断
	EXTI_SetExtIntSensitivity(EXTI_PORT_GPIOD, EXTI_SENSITIVITY_RISE_FALL);			   // 端口D 仅下降沿触发
	GPIO_Init(GPIOA, GPIO_PIN_3, GPIO_MODE_OUT_PP_LOW_FAST);						   // 初始化 TIM2 通道3 输出125KHz 的PWM 50%占空比的脉冲信号 ，PA3 引脚输出

	TIM2->ARRH = 0;	
	TIM2->ARRL = 0x3F;	
	TIM2->CCR3L = 0x1F;
	TIM2->PSCR = 1;
	TIM2->CCER2 = 1;
	TIM2->CCMR3 = 0x70;
	TIM2->CR1 = 1;

	// TIM2_DeInit();																	   // 使能定时器2
	// TIM2_TimeBaseInit(TIM2_PRESCALER_2, 63);										   // 2分频，125khz
	// TIM2_OC3Init(TIM2_OCMODE_PWM2, TIM2_OUTPUTSTATE_ENABLE, 31, TIM2_OCPOLARITY_HIGH); // 占空比50%
	// TIM2_Cmd(ENABLE);																   // 使能定时器2
}
#define Half_Min 3	// 150us
#define Half_Max 7	// 350us
#define Full_Min 8	// 400us
#define Full_Max 13 // 650us
/**
 * checkCardData
 * @brief id卡卡号校验
 * @author Honokahqh
 * @date 2024-1-10
 */
void memset(uint8_t *data, uint8_t value, uint16_t len)
{
	for (int i = 0; i < len; i++)
	{
		data[i] = value;
	}
}
int checkCardData(uint8_t *data, uint16_t bitNum)
{
	// 行校验
	for (int row = 0; row < (bitNum / 5) - 1; row++)
	{
		int rowSum = 0;
		for (int bit = 0; bit < 4; bit++)
		{
			rowSum += (data[(row * 5 + bit + 9) / 8] >> ((row * 5 + bit + 9) % 8)) & 1;
		}
		rowSum += (data[(row * 5 + 4 + 9) / 8] >> ((row * 5 + 4 + 9) % 8)) & 1; // 加上Px
		if (rowSum % 2 != 0)
		{
			return 0; // 行校验失败
		}
	}

	// 列校验
	for (int col = 0; col < 4; col++)
	{
		int colSum = 0;
		for (int row = 0; row < (bitNum / 5) - 1; row++)
		{
			colSum += (data[(row * 5 + col + 9) / 8] >> ((row * 5 + col + 9) % 8)) & 1;
		}
		colSum += (data[(59 + col) / 8] >> ((59 + col) % 8)) & 1; // 加上PCx
		if (colSum % 2 != 0)
		{
			return 0; // 列校验失败
		}
	}

	//
	memset((uint8_t *)&ID_Card_Info, 0, sizeof(ID_Card_Info_t));
	for (int i = 0; i < (bitNum / 5) - 1; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			int bit = (data[((i * 5) + j + 9) / 8] & (1 << ((i * 5 + j + 9) % 8))) != 0; // 读取位
			if (bit)
			{
				int originalPos = (i * 4 + j);									   // 计算原始位置
				int reversedPos = (originalPos / 8) * 8 + (7 - (originalPos % 8)); // 计算反转后的位置
				ID_Card_Info.ID[reversedPos / 8] |= 1 << (reversedPos % 8);
			}
		}
	}

	// 所有校验通过
	return 1;
}

uint8_t get_id_io_state()
{
	if (GPIOD->IDR & GPIO_PIN_3)
		return 0;
	else
		return 1;
}
void id_card_decode()
{
	if (ID_Card.state == ID_IDLE)
	{
		if (sys_50us_id >= Full_Min && sys_50us_id <= Full_Max && get_id_io_state()) // 必然是一个翻转信号且为1
		{
			ID_Card.state = ID_START;
			ID_Card.ID[0] = 1;
		}
	}
	else if (ID_Card.state == ID_START)
	{
		if (sys_50us_id >= Half_Min && sys_50us_id <= Half_Max) // 16个空跳
		{
			ID_Card.count++;
			if (ID_Card.count == 16)
			{
				ID_Card.state = ID_DATA;
				ID_Card.count = 0;
				ID_Card.ID[0] = 0xFF;
				ID_Card.ID[1] = 1;
				ID_Card.bit = 9;
			}
		}
		else
		{
			memset((uint8_t *)&ID_Card, 0, sizeof(ID_Card_t));
		}
	}
	else if (ID_Card.state == ID_DATA) // (4 + 1)*10 + 4 + 1
	{
		if (sys_50us_id >= Full_Min && sys_50us_id <= Full_Max)
		{
			ID_Card.value = get_id_io_state();
			ID_Card.ID[ID_Card.bit / 8] |= (ID_Card.value << (ID_Card.bit % 8));
			ID_Card.bit++;
			ID_Card.count = 0;
		}
		else if (sys_50us_id >= Half_Min && sys_50us_id <= Half_Max)
		{
			ID_Card.count++;
			if (ID_Card.count % 2 == 0)
			{
				ID_Card.value = get_id_io_state();
				ID_Card.ID[ID_Card.bit / 8] |= (ID_Card.value << (ID_Card.bit % 8));
				ID_Card.bit++;
			}
		}
		else
		{
			memset((uint8_t *)&ID_Card, 0, sizeof(ID_Card_t));
		}
	}
	if (ID_Card.bit == 9 + 5 * 11) // 64bitnum 常见RFID卡标准
	{
		if (checkCardData((uint8_t *)ID_Card.ID, 5 * 11)) // !check的时间超过了500us,导致下一次中断进不来
		{
			ID_Card_Info.has_card = 1;
		}
		memset((uint8_t *)&ID_Card, 0, sizeof(ID_Card_t));
	}
	sys_50us_id = 0;
}

INTERRUPT_HANDLER(EXTI_PORTD_IRQHandler, 6)
{
	id_card_decode();
}
