#include "stm32f4xx_conf.h"

#include "libc.h"
#include "console.h"
#include "event.h"

#define	USARTx	USART3

void sendchar(int val) {
	while (USART_GetFlagStatus(USARTx, USART_FLAG_TXE) == RESET);
	USART_SendData(USARTx, val);
}

int getchar(void) {
	int	ch;

	while (USART_GetFlagStatus(USARTx, USART_FLAG_RXNE) == RESET);
	ch = USART_ReceiveData(USARTx);

	return ch;
}

void usart3_init(void) {
	GPIO_InitTypeDef	GPIO_InitStructure;
	USART_InitTypeDef	USART_InitStructure;
	NVIC_InitTypeDef	NVIC_InitStructure;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);

	GPIO_PinAFConfig(GPIOD, GPIO_PinSource8, GPIO_AF_USART3);	// TX
	GPIO_PinAFConfig(GPIOD, GPIO_PinSource9, GPIO_AF_USART3);	// RX

	/* Configure USART Tx as alternate function  */
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	/* Configure USART Rx as alternate function  */
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

//	USART_InitStructure.USART_BaudRate = 9600;
	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

	USART_Init(USART3, &USART_InitStructure);

	NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);

	USART_Cmd(USART3, ENABLE);
}

void usart3_handle_it(void) {
	int	ch;

	ch = USART_ReceiveData(USART3);

	if (!con_input_islocked()) {

		if (ch == 13) {	// \n
			con_input_lock();
			event_push(E_CON_ECHO, '\n', NULL);
			event_push(E_CON_INPUT, 0, NULL);
		} else {
			if (ch == 8 && con_input_hasinput()) {
				con_input_del();
				event_push(E_CON_DEL, 0, NULL);
			} else {
				con_input_push(ch);
				event_push(E_CON_ECHO, ch, NULL);
			}
		}
	}
	// else discard data
}

void host_con_init(void) {
	usart3_init();
	xdev_out(sendchar);
}

void host_con_poll(void) {
}
