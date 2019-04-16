#include "stm32f4_discovery.h"
#include "stm32f4xx_conf.h"
#include "stm32f4x7_eth.h"

#include "lwipopts.h"
#include "lwip/init.h"
#include "lwip/dhcp.h"
#include "lwip/dns.h"
#include "lwip/netif.h"
#include "lwip/ip_addr.h"
#include "lwip/tcp.h"
#include "netif/etharp.h"

#include "libc.h"
#include "misc.h"
#include "stm32.h"
#include "iobuffer.h"
#include "platform.h"

#include "ethernetif.h"

#ifdef	CONFIG_NET_DEBUG
#define	net_debug(...)	xprintf(__VA_ARGS__)
#else
#define	net_debug(...)
#endif

///////////////////////////////////////////////////////////////////////////////

#define LAN8720_PHY_ADDRESS       0x01	/* Relative to STM324xG-EVAL Board */

extern struct netif	*netif_list;

enum eth_state_e {
	ETH_UNKNOWN = 0,
	ETH_INIT,
	ETH_FAILED,
};

struct {
	ip_addr_t		server;
	struct netif		netif;
	struct tcp_pcb		*pcb;

	char			hostname[MAX_HOSTNAME];
	char			filename[MAX_URL];
	int			port;

	enum eth_state_e	eth_state;
	enum net_state_e	link_state;

	uint8_t			led;
} static ns;

/* Nowhere declared in LWIP */
void sys_check_timeouts(void);

///// ETH BSP initialization ///////////////////////////////////////////////////

/**
  * @brief  Configures the different GPIO ports.
  * @param  None
  * @retval None
  */
void ETH_GPIO_Config(void) {
	GPIO_InitTypeDef	GPIO_InitStructure;
	volatile uint32_t	i;

	/* Enable GPIOs clocks */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOB
                      | RCC_AHB1Periph_GPIOC, ENABLE);

	/* Enable SYSCFG clock */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

	/* MII/RMII Media interface selection --------------------------------------*/
	SYSCFG_ETH_MediaInterfaceConfig(SYSCFG_ETH_MediaInterface_RMII);


	/* Ethernet pins configuration ************************************************/
	/*
	ETH_MDIO --------------> PA2
	ETH_MDC ---------------> PC1

	ETH_RMII_REF_CLK-------> PA1

	ETH_RMII_CRS_DV -------> PA7
	ETH_MII_RX_ER   -------> PB10
	ETH_RMII_RXD0   -------> PC4
	ETH_RMII_RXD1   -------> PC5
	ETH_RMII_TX_EN  -------> PB11
	ETH_RMII_TXD0   -------> PB12
	ETH_RMII_TXD1   -------> PB13

	ETH_RST_PIN     -------> PE2
	*/

	/* Configure PA1,PA2 and PA7 */
	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL ;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource1, GPIO_AF_ETH);
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_ETH);
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource7, GPIO_AF_ETH);

	/* Configure PB10,PB11,PB12 and PB13 */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource10, GPIO_AF_ETH);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource11, GPIO_AF_ETH);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource12, GPIO_AF_ETH);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource13, GPIO_AF_ETH);

	/* Configure PC1, PC4 and PC5 */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_4 | GPIO_Pin_5;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	GPIO_PinAFConfig(GPIOC, GPIO_PinSource1, GPIO_AF_ETH);
	GPIO_PinAFConfig(GPIOC, GPIO_PinSource4, GPIO_AF_ETH);
	GPIO_PinAFConfig(GPIOC, GPIO_PinSource5, GPIO_AF_ETH);

	/* Configure the PHY RST  pin */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOE, &GPIO_InitStructure);

	GPIO_ResetBits(GPIOE, GPIO_Pin_2);
	for (i = 0; i < 20000; i++);
	GPIO_SetBits(GPIOE, GPIO_Pin_2);
	for (i = 0; i < 20000; i++);
}


/**
  * @brief  Configures the Ethernet Interface
  * @param  None
  * @retval None
  */
static uint32_t ETH_MACDMA_Config(void) {
	ETH_InitTypeDef ETH_InitStructure;
	uint32_t	EthInitStatus;

	/* Enable ETHERNET clock  */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_ETH_MAC | RCC_AHB1Periph_ETH_MAC_Tx |
                        RCC_AHB1Periph_ETH_MAC_Rx, ENABLE);

	/* Reset ETHERNET on AHB Bus */
	ETH_DeInit();

	/* Software reset */
	ETH_SoftwareReset();

	/* Wait for software reset */
	while (ETH_GetSoftwareResetStatus() == SET);

	/* ETHERNET Configuration --------------------------------------------------*/
	/* Call ETH_StructInit if you don't like to configure all ETH_InitStructure parameter */
	ETH_StructInit(&ETH_InitStructure);

	/* Fill ETH_InitStructure parametrs */
	/*------------------------   MAC   -----------------------------------*/
	ETH_InitStructure.ETH_AutoNegotiation = ETH_AutoNegotiation_Enable;
	//ETH_InitStructure.ETH_AutoNegotiation = ETH_AutoNegotiation_Disable;
	//  ETH_InitStructure.ETH_Speed = ETH_Speed_10M;
	//  ETH_InitStructure.ETH_Mode = ETH_Mode_FullDuplex;

	ETH_InitStructure.ETH_LoopbackMode = ETH_LoopbackMode_Disable;
	ETH_InitStructure.ETH_RetryTransmission = ETH_RetryTransmission_Disable;
	ETH_InitStructure.ETH_AutomaticPadCRCStrip = ETH_AutomaticPadCRCStrip_Disable;
	ETH_InitStructure.ETH_ReceiveAll = ETH_ReceiveAll_Disable;
	ETH_InitStructure.ETH_BroadcastFramesReception = ETH_BroadcastFramesReception_Enable;
	ETH_InitStructure.ETH_PromiscuousMode = ETH_PromiscuousMode_Disable;
	ETH_InitStructure.ETH_MulticastFramesFilter = ETH_MulticastFramesFilter_Perfect;
	ETH_InitStructure.ETH_UnicastFramesFilter = ETH_UnicastFramesFilter_Perfect;
#ifdef CHECKSUM_BY_HARDWARE
	ETH_InitStructure.ETH_ChecksumOffload = ETH_ChecksumOffload_Enable;
#endif

	/*------------------------   DMA   -----------------------------------*/

	/* When we use the Checksum offload feature, we need to enable the Store and Forward mode:
	the store and forward guarantee that a whole frame is stored in the FIFO, so the MAC can insert/verify the checksum,
	if the checksum is OK the DMA can handle the frame otherwise the frame is dropped */
	ETH_InitStructure.ETH_DropTCPIPChecksumErrorFrame = ETH_DropTCPIPChecksumErrorFrame_Enable;
	ETH_InitStructure.ETH_ReceiveStoreForward = ETH_ReceiveStoreForward_Enable;
	ETH_InitStructure.ETH_TransmitStoreForward = ETH_TransmitStoreForward_Enable;

	ETH_InitStructure.ETH_ForwardErrorFrames = ETH_ForwardErrorFrames_Disable;
	ETH_InitStructure.ETH_ForwardUndersizedGoodFrames = ETH_ForwardUndersizedGoodFrames_Disable;
	ETH_InitStructure.ETH_SecondFrameOperate = ETH_SecondFrameOperate_Enable;
	ETH_InitStructure.ETH_AddressAlignedBeats = ETH_AddressAlignedBeats_Enable;
	ETH_InitStructure.ETH_FixedBurst = ETH_FixedBurst_Enable;
	ETH_InitStructure.ETH_RxDMABurstLength = ETH_RxDMABurstLength_32Beat;
	ETH_InitStructure.ETH_TxDMABurstLength = ETH_TxDMABurstLength_32Beat;
	ETH_InitStructure.ETH_DMAArbitration = ETH_DMAArbitration_RoundRobin_RxTx_2_1;

	/* Configure Ethernet */
	EthInitStatus = ETH_Init(&ETH_InitStructure, LAN8720_PHY_ADDRESS);

	return (EthInitStatus);
}

///// LWIP and ETH interface initialization ///////////////////////////////////

void net_lwip_init(void) {
	ip_addr_t	ipaddr;
	ip_addr_t	netmask;
	ip_addr_t	gw;

	lwip_init();

	uc_memset(&ipaddr, 0, sizeof(ipaddr));
	uc_memset(&netmask, 0, sizeof(ipaddr));
	uc_memset(&gw, 0, sizeof(gw));

#if	LWIP_DHCP==0
	IP4_ADDR(&ipaddr, IP_ADDR0, IP_ADDR1, IP_ADDR2, IP_ADDR3);
	IP4_ADDR(&netmask, NET_MASK0, NET_MASK1, NET_MASK2, NET_MASK3);
	IP4_ADDR(&gw, GW_ADDR0, GW_ADDR1, GW_ADDR2, GW_ADDR3);

	net_debug("Static IP: %d.%d.%d.%d ",
		  IP_ADDR0, IP_ADDR1, IP_ADDR2, IP_ADDR3);
#endif

	netif_add(&ns.netif, &ipaddr, &netmask, &gw, NULL, &ethernetif_init,
		  &ethernet_input);
	netif_set_default(&ns.netif);
	netif_set_up(&ns.netif);

#if	LWIP_DHCP==1
	dhcp_start(&ns.netif);
#endif
}

void net_timers(void) {

	if (ETH_CheckFrameReceived()) {
		ethernetif_input(&ns.netif);
	}
//	if (ns.eth_state == ETH_FAILED)
//		return;

	sys_check_timeouts();
}

///// lwip callbacks //////////////////////////////////////////////////////////


int sys_now(void) {
	return get_local_time();
}

int net_lwip_dhcp_tries(void) {
#if	LWIP_DHCP
	struct dhcp *dhcp = netif_dhcp_data(&ns.netif);

	return dhcp->tries;
#else
	return 0;
#endif
}

int net_lwip_build_buffer(struct tcp_pcb *tpcb, struct pbuf *p, void *buf,
			  int buf_len) {
	struct pbuf	*tmp_p;
	int		tmp_len;

	if (p == NULL) {
		net_debug("%s: got a null pbuf.\n", __func__);
		tmp_len = 0;
		goto error;
	}

	if (p->tot_len > buf_len) {
		net_debug
		    ("%s: received data do not fit into buffer (%d > %d).\n",
		     __func__, p->tot_len, buf_len);
		tmp_len = -1;
		goto error;
	}

	uc_memset(buf, 0, buf_len);

	for (tmp_p = p, tmp_len = 0; tmp_p != NULL; tmp_p = tmp_p->next) {
		uc_memcpy(buf + tmp_len, tmp_p->payload, tmp_p->len);
		tmp_len += tmp_p->len;
	}

	goto ack_len;

error:
	ns.link_state = NET_ERROR;

ack_len:
	tcp_recved(tpcb, p->tot_len);
	pbuf_free(p);

	return (tmp_len);
}

void cmd_io_state(void);

err_t net_lwip_recv(void *arg, struct tcp_pcb * tpcb, struct pbuf * p,
		    err_t err) {
	struct input_s	*in;
	uint8_t		buf[TCP_MSS];
	int		free = i_free();
	int		ret;

	if (!free) {
		net_debug("%s: no free space on input.\n", __func__);
		tcp_recved(tpcb, p->tot_len);
		pbuf_free(p);
		cmd_io_state();
		return err;
	}

	if (err != ERR_OK) {
		net_debug("%s: got an error as a parameter.\n", __func__);
		ns.link_state = NET_ERROR;
		return err;
	}

	ret = net_lwip_build_buffer(tpcb, p, &buf, sizeof(buf));

	if (ret <= 0) {
		net_debug("%s: build buffer error (%d).\n", __func__, ret);
		return err;
	}

	if (ret > IO_INPUT_BUFFER_SIZE) {
		ret = IO_INPUT_BUFFER_SIZE;
		net_debug("%s: overflowing data on input.\n", __func__);
	}

	in = i_push();
	uc_memcpy(in->data, buf, ret);
	in->used = ret;

	ns.led = 1 - ns.led;
	if (ns.led) {
		STM_EVAL_LEDOn(LED_NET);
	} else {
		STM_EVAL_LEDOff(LED_NET);
	}

	return ERR_OK;
}

err_t net_lwip_recv_connect(void *arg, struct tcp_pcb * tpcb, struct pbuf * p,
			    err_t err) {
	uint8_t	buf[TCP_MSS];
	int	ret;

	net_debug("%s\n", __func__);

	if (err != ERR_OK) {
		net_debug
		    ("%s: got an error as a parameter.\n", __func__);
		ns.link_state = NET_ERROR;
		return err;
	}

	ret = net_lwip_build_buffer(tpcb, p, &buf, sizeof(buf));

	if (ret <= 0) {
		net_debug("%s: build buffer error (%d).\n",
			__func__, ret);
		return err;
	}

	net_debug("%s: response len is %d\n", __func__, uc_strlen((const char *)&buf));
	net_debug("%s: response \n--------------------\n%s\n\
				\n--------------------\n",
				__func__, buf);
	net_debug("%s: connection is open\n", __func__);

	if (uc_strstr((char *) buf, "404") != NULL) {
		xprintf("%s: no media found, got 404.\n", __func__);
		ns.link_state = NET_ERROR;
		return err;
	}

	if (uc_strstr((char *) buf, "application/ogg") != NULL) {
		xprintf("%s: OGG is not supported.\n", __func__);
		ns.link_state = NET_ERROR;
		return err;
	}

	if (uc_strstr((char *) buf, "audio/mpeg") == NULL) {
		xprintf("%s: unknown media type.\n", __func__);
		xprintf("'%s'\n", buf);
		ns.link_state = NET_ERROR;
		return err;
	}

	ns.link_state = NET_OPEN;
	net_debug("%s: got media I want.\n", __func__);

	tcp_recv(ns.pcb, net_lwip_recv);

	return ERR_OK;
}

err_t net_lwip_poll(void *arg, struct tcp_pcb * pcb) {

	net_debug("%s\n", __func__);

	return ERR_OK;
}

err_t net_lwip_sent(void *arg, struct tcp_pcb * tpcb, uint16_t len) {

	net_debug("%s (%d)\n", __func__, len);

	return ERR_OK;
}

err_t net_lwip_connected(void *arg, struct tcp_pcb * pcb, err_t err) {
	char	httprequest[512];
	int	len;
	err_t	ret;

	net_debug("%s\n", __func__);

	if (err != ERR_OK) {
		net_debug("However connected with error.\n");
		ns.link_state = NET_ERROR;
		return err;
	}

	uc_memset(httprequest, 0, sizeof(httprequest));

	xsprintf(httprequest,
		 "GET %s HTTP/1.0\r\n"
		 "Pragma: no-cache\r\n"
		 "Host: %s\r\n"
		 "User-Agent: xmms/1.2.7\r\n"
		 "Accept: * / *\r\n" "\r\n", ns.filename, ns.hostname);

	len = uc_strlen((char *) httprequest);

	net_debug("%s: request len: %d\n", __func__, len);

	net_debug("%s: request: \n%s\n", __func__, httprequest);

	tcp_arg(ns.pcb, NULL);
	tcp_recv(ns.pcb, net_lwip_recv_connect);
	tcp_sent(ns.pcb, net_lwip_sent);
//	tcp_poll(ns.pcb, net_lwip_poll, 1);

	ret = tcp_write(ns.pcb, httprequest, len, 1);

	return ret;
}

void net_lwip_open(void) {

	net_debug("%s\n", __func__);

	ns.pcb = tcp_new();
	if (ns.pcb == NULL) {
		xprintf("%s: pcb allocation failed.\n", __func__);
		ns.link_state = NET_ERROR;
		return;
	}

	tcp_connect(ns.pcb, &ns.server, ns.port, net_lwip_connected);
}

void net_lwip_close(void) {

	ns.link_state = NET_CLOSED;

	tcp_recv(ns.pcb, NULL);
	tcp_sent(ns.pcb, NULL);
	tcp_poll(ns.pcb, NULL, 0);

	tcp_close(ns.pcb);

	net_debug("%s\n", __func__);
}

void net_lwip_dns(const char *name, const ip_addr_t *ipaddr, void *callback_arg) {

	net_debug("%s: ", __func__);

	if (ipaddr != NULL && ipaddr->addr != 0) {
		ns.server.addr = ipaddr->addr;
		net_debug("OK\n");
		net_lwip_open();
	} else {
		net_debug("Error resolving DNS.\n");
		ns.link_state = NET_ERROR;
	}
}

///// Mardio network API //////////////////////////////////////////////////////

void host_net_init(void) {
	uint32_t	ret;

	net_debug("%s: %s ", __func__, LWIP_VERSION_STRING);

	uc_memset(&ns, 0, sizeof(ns));

	ETH_GPIO_Config();

	ret = ETH_MACDMA_Config();

	if (ret == 0) {
		net_debug("Failed!\n");
		ns.eth_state = ETH_FAILED;
	} else {
		net_lwip_init();
		net_debug("OK\n");
		ns.eth_state = ETH_INIT;
		ns.link_state = NET_CLOSED;
	}
}

void host_net_done(void) {
}

void host_net_open(const char *url) {
//	err_t	ret = ERR_OK;
	err_t	ret;

	if (ns.eth_state == ETH_ERROR)
		return;

	net_debug("%s '%s'\n", __func__, url);

	uc_memset(ns.hostname, 0, sizeof ns.hostname);
	uc_memset(ns.filename, 0, sizeof ns.filename);

	net_parse_url(url, ns.hostname, &ns.port, ns.filename);

	ret = dns_gethostbyname(ns.hostname, &ns.server, net_lwip_dns, NULL);

	switch (ret) {

	case ERR_OK:
		net_debug("%s: DNS resolved, opening connection\n", __func__);
		ns.link_state = NET_PENDING;
		net_lwip_open();
		break;

	case ERR_INPROGRESS:
		net_debug("%s: DNS is querring for data\n", __func__);
		ns.link_state = NET_PENDING;
		break;

	case ERR_ARG:
		net_debug("%s: Error arguments for dns_gethostbyname\n", __func__);
		ns.link_state = NET_ERROR;
		break;

	default:
		xprintf("%s: Any other random error %d\n", __func__, ret);
		ns.link_state = NET_ERROR;
	}

}

void host_net_close(void) {
	net_lwip_close();
	net_debug("%s\n", __func__);
}

void host_net_poll(void) {

	if (ns.eth_state == ETH_FAILED)
		return;

#if	LWIP_DHCP==1
	if ((net_lwip_dhcp_tries() > 4) && (ns.netif.ip_addr.addr == 0)) {
		net_debug("%s: dhcp request timed out.\n", __func__);
		ns.eth_state = ETH_FAILED;
	}
#endif
}

enum net_state_e host_net_state(void) {
	return (ns.link_state);
}

///// Command line interface //////////////////////////////////////////////////

void cmd_set_addr(void) {
	ip_addr_t	ipaddr;
	ip_addr_t	netmask;
	ip_addr_t	gw;

	IP4_ADDR(&ipaddr, IP_ADDR0, IP_ADDR1, IP_ADDR2, IP_ADDR3);
	IP4_ADDR(&netmask, NET_MASK0, NET_MASK1, NET_MASK2, NET_MASK3);
	IP4_ADDR(&gw, GW_ADDR0, GW_ADDR1, GW_ADDR2, GW_ADDR3);

	netif_set_addr(&ns.netif, &ipaddr, &netmask, &gw);
	dhcp_stop(&ns.netif);

	net_debug("Static IP: %d.%d.%d.%d\n",
		  IP_ADDR0, IP_ADDR1, IP_ADDR2, IP_ADDR3);

}

void cmd_net_state(void) {

	xprintf("ETH link is ");
	switch (ns.eth_state) {
	case ETH_INIT:
		xprintf("initalized.");
		break;
	case ETH_FAILED:
		xprintf("in error state.\n");
		break;
	case ETH_UNKNOWN:
	default:
		xprintf("uninitialized.\n");
	}
	xprintf("\n");

	if (ns.eth_state != ETH_INIT)
		return;

	xprintf("Network connection ");
	switch (ns.link_state) {
	case NET_CLOSED:
		xprintf("is closed");
		break;
	case NET_OPEN:
		xprintf("is open");
		break;
	case NET_ERROR:
		xprintf("encountered error");
		break;
	case NET_PENDING:
		xprintf("is waiting for response");
		break;
	default:
		xprintf("encountered other error");
	}
	xprintf(".\n");

	xprintf("\n");
}

void net_ifconfig(struct netif *netif) {
	int	i, comma;

	struct {
		const uint8_t flag;
		const char *name;
	} const if_flags[] = {
		{NETIF_FLAG_UP, "UP"},
		{NETIF_FLAG_BROADCAST, "BROADCAST"},
		{NETIF_FLAG_LINK_UP, "LINK"},
		{NETIF_FLAG_ETHARP, "ARP"},
		{NETIF_FLAG_IGMP, "IGMP"},
		{NETIF_FLAG_MLD6, "MDL6"},
		{0, NULL},
	};


	xprintf("%c%c: ", netif->name[0], netif->name[1]);

	xprintf("flags=%d<", netif->flags);

	for (i = 0, comma = 0; if_flags[i].name != NULL; i++) {

		if (netif->flags & if_flags[i].flag) {
			if (comma)
				xprintf(",");
			xprintf("%s", if_flags[i].name);
			comma++;
		}
	}
	xprintf("> ");

	xprintf("mtu %d\n", netif->mtu);

	xprintf("    inet %d.%d.%d.%d netmask %d.%d.%d.%d gw %d.%d.%d.%d\n",
		(netif->ip_addr.addr >> 0) & 0xFF,
		(netif->ip_addr.addr >> 8) & 0xFF,
		(netif->ip_addr.addr >> 16) & 0xFF,
		(netif->ip_addr.addr >> 24) & 0xFF,
		(netif->netmask.addr >> 0) & 0xFF,
		(netif->netmask.addr >> 8) & 0xFF,
		(netif->netmask.addr >> 16) & 0xFF,
		(netif->netmask.addr >> 24) & 0xFF,
		(netif->gw.addr >> 0) & 0xFF,
		(netif->gw.addr >> 8) & 0xFF,
		(netif->gw.addr >> 16) & 0xFF,
		(netif->gw.addr >> 24) & 0xFF);

	xprintf("    ether %02x:%02x:%02x:%02x:%02x:%02x",
		netif->hwaddr[0], netif->hwaddr[1], netif->hwaddr[2],
		netif->hwaddr[3], netif->hwaddr[4], netif->hwaddr[5]);

#if LWIP_DHCP == 1
	xprintf(" dhcp tries %d", net_lwip_dhcp_tries());
#endif

	xprintf("\n");

	xprintf("    RX packets %d  bytes %d (%d KB)\n",
		ns.netif.rx_count, ns.netif.rx_size, (ns.netif.rx_size / 1024));

	xprintf("    TX packets %d  bytes %d (%d KB)\n",
		ns.netif.tx_count, ns.netif.tx_size, (ns.netif.tx_size / 1024));

	xprintf("\n");
}

void cmd_net_ifconfig(void) {
	struct netif	*netif;

	for (netif = netif_list; netif != NULL; netif = netif->next) {
		net_ifconfig(netif);
	}
}
