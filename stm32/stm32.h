#ifndef	_STM32_H_
#define	_STM32_H_

#include <stdint.h>
#include "stm32f4_discovery.h"

#define	MAC_ADDR0	0x80
#define	MAC_ADDR1	0x86
#define	MAC_ADDR2	0x80
#define	MAC_ADDR3	0x88
#define	MAC_ADDR4	0x03
#define	MAC_ADDR5	0x86

#define	IP_ADDR0	10
#define	IP_ADDR1	0
#define	IP_ADDR2	0
#define	IP_ADDR3	10

#define	NET_MASK0	255
#define	NET_MASK1	255
#define	NET_MASK2	255
#define	NET_MASK3	0

#define	GW_ADDR0	10
#define	GW_ADDR1	0
#define	GW_ADDR2	0
#define	GW_ADDR3	1

#define	LED_FS		LED3
#define	LED_SYSTEM	LED4
#define	LED_SOUND	LED5
#define	LED_NET		LED6

void usart3_handle_it(void);
void timer_handle_it(void);

void usb_init(void);
void usb_loop(void);
int usb_ispresent(void);

void fs_mount(void);
void fs_umount(void);
int fs_ismounted(void);
void fs_poll_usb(void);

void net_timers(void);

void rng_init(void);
uint32_t rng_rand(void);

void Delay(uint32_t nCount);
#endif
