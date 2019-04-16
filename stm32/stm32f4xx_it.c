/**
  ******************************************************************************
  * @file    SysTick/stm32f4xx_it.c 
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    19-September-2011
  * @brief   Main Interrupt Service Routines.
  *          This file provides template for all exceptions handler and 
  *          peripherals interrupt service routine.
  ******************************************************************************
  * @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2011 STMicroelectronics</center></h2>
  ******************************************************************************
  */ 

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_it.h"
#include "stm32f4xx_conf.h"

#include "usb_core.h"
#include "usb_hcd_int.h"

extern USB_OTG_CORE_HANDLE	USB_OTG_Core;

void USB_OTG_BSP_TimerIRQ (void);

void usart3_handle_it (void);
void timer_handle_it (void);
void usb_over_current (void);

void NMI_Handler(void) {
}

void HardFault_Handler(void) {
	/* Go to infinite loop when Hard Fault exception occurs */
	while (1) {
	}
}

void MemManage_Handler(void) {
	/* Go to infinite loop when Memory Manage exception occurs */
	while (1) {
	}
}

void BusFault_Handler(void) {
	/* Go to infinite loop when Bus Fault exception occurs */
	while (1) {
	}
}

void UsageFault_Handler(void) {
	/* Go to infinite loop when Usage Fault exception occurs */
	while (1) {
	}
}

void SVC_Handler(void) {
}

void DebugMon_Handler(void) {
}

void PendSV_Handler(void) {
}

void SysTick_Handler(void) {
	timer_handle_it ();
}

void USART3_IRQHandler (void) {
	usart3_handle_it ();
}

void TIM2_IRQHandler(void) {
	USB_OTG_BSP_TimerIRQ();
}

void OTG_FS_IRQHandler(void) {
	USBH_OTG_ISR_Handler(&USB_OTG_Core);
}

void EXTI9_5_IRQHandler(void) {
	usb_over_current ();
}

/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
