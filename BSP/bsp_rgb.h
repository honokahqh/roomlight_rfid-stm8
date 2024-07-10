#ifndef _BSP_RGB_H
#define _BSP_RGB_H

#include "stm8s.h"
#include "bsp_rgb.h"
#include "bsp_stEEPROM.h"
#include "stm8s_exti.h"
#include "stm8s_gpio.h"
#include "stm8s_tim2.h"
// PA2 RS485 EN
// PA3 RFID PWM
// PC3 PWM	B
// PC4 PWM	R
// PC6 PWM	G	
// PD3 RFID DATA
// PD2 BELL

void bsp_driver_init(void);

extern volatile uint32_t sysTime;
void delay_ms(uint16_t ms);

void bsp_dog_reload(void);


#define _RS485_RX() GPIOA->ODR &= ~GPIO_PIN_2
#define _RS485_TX() GPIOA->ODR |= GPIO_PIN_2
#define UART_IDLE_TIMEOUT 5
#define UART_BUF_MAX 64
typedef struct
{
    uint8_t IDLE;     // 串口空闲-0:空闲
    uint8_t has_data; // 串口一帧数据接收完成

    uint8_t rx_buff[UART_BUF_MAX];
    uint8_t rx_len;
} uart_state_t;
extern uart_state_t uart_state;
void mbs_sendData(uint8_t *_str, uint8_t _strLen);

void bsp_rgb_load565(uint16_t color565);

typedef enum
{
	ID_IDLE = 0,
	ID_START,
	ID_DATA,
} ID_Card_State_t;

typedef struct
{
	uint8_t ID[50]; // 64
	uint16_t bit;	// bit num
	uint8_t value;
	uint8_t count;
	ID_Card_State_t state;
} ID_Card_t;
extern volatile ID_Card_t ID_Card;

typedef struct
{
	/* data */
	uint8_t ID[6];
	uint8_t has_card;
} ID_Card_Info_t;
extern volatile ID_Card_Info_t ID_Card_Info;

#endif
