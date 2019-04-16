#######################################

TARGET=mardio

#######################################

DEF_DEBUG=\
		-DCONFIG_SOUND_DEBUG\
		-DCONFIG_LIBMAD_DEBUG\
		-DCONFIG_SALLOC_DEBUG\
		-DCONFIG_LCD_DEBUG\
		-DCONFIG_NET_DEBUG\
		-DCONFIG_USB_DEBUG\
		-DCONFIG_FS_DEBUG\
		-DCONFIG_RESOURCES_DEBUG\
		-DCONFIG_IOBUFFER_DEBUG\
		-DCONFIG_PLAYER_DEBUG\

#######################################

OBJ_COMMON=	common/console.o\
		common/cmd.o\
		common/event.o\
		common/fs.o\
		common/iobuffer.o\
		common/lcd.o\
		common/libmad.o\
		common/net.o\
		common/player.o\
		common/resfs.o\
		common/snd.o\
		common/version.o\

INC_COMMON=	-Icommon

RM_COMMON=	common/*.o

#DEF_COMMON=	-DHAVE_RESOURCES
DEF_COMMON=

CFL_COMMON=	-g\
		-O2\
		-Wall

#######################################

OBJ_ID3TAG=	libs/id3tag/id3tag.o

INC_ID3TAG=	-Ilibs/id3tag

RM_ID3TAG=	libs/id3tag/*.o

DEF_ID3TAG=

CFL_ID3TAG=

#######################################

OBJ_LIBC=	libs/libc/strings.o\
		libs/libc/xprintf.o\
		libs/libc/salloc.o\
		libs/libc/libc.o\

INC_LIBC=	-Ilibs/libc

RM_LIBC=	libs/libc/*.o

DEF_LIBC=

CFL_LIBC=

#######################################

OBJ_LINUX=	linux/console.o\
		linux/fs.o\
		linux/lcd.o\
		linux/main.o\
		linux/net.o\
		linux/sound.o\
		linux/timer.o\

INC_LINUX=	-Ilinux\
		-I/usr/include\
		-Icommon\

RM_LINUX=	linux/*.o

DEF_LINUX=	-D__LINUX__\
		-DNO_LCD

CFL_LINUX=	$(shell sdl-config --cflags)

LFL_LINUX=	-lmad\
		-lasound\
		-lc\
		$(shell sdl-config --libs)

#######################################

ifeq ($(PL),linux)

CROSS=

CC=$(CROSS)gcc
LD=$(CROSS)gcc
OJ=$(CROSS)objcopy

OBJ=	$(OBJ_LIBC)\
	$(OBJ_ID3TAG)\
	$(OBJ_COMMON)\
	$(OBJ_LINUX)

INC=	$(INC_LIBC)\
	$(INC_ID3TAG)\
	$(INC_COMMON)\
	$(INC_LINUX)

DEF=	$(DEF_LIBC)\
	$(DEF_ID3TAG)\
	$(DEF_COMMON)\
	$(DEF_LINUX)

CFL=	$(CFL_LIBC)\
	$(CFL_ID3TAG)\
	$(CFL_COMMON)\
	$(CFL_LINUX)

LFL=	$(LFL_LIBC)\
	$(LFL_ID3TAG)\
	$(LFL_COMMON)\
	$(LFL_LINUX)

endif

#######################################

STMDDIR=libs/STM32F4xx_StdPeriph_Driver
DISCDIR=libs/STM32F4-Discovery

OBJ_STMD=	$(STMDDIR)/src/misc.o\
		$(STMDDIR)/src/stm32f4xx_dac.o\
		$(STMDDIR)/src/stm32f4xx_dma.o\
		$(STMDDIR)/src/stm32f4xx_exti.o\
		$(STMDDIR)/src/stm32f4xx_gpio.o\
		$(STMDDIR)/src/stm32f4xx_i2c.o\
		$(STMDDIR)/src/stm32f4xx_rcc.o\
		$(STMDDIR)/src/stm32f4xx_rng.o\
		$(STMDDIR)/src/stm32f4xx_sdio.o\
		$(STMDDIR)/src/stm32f4xx_spi.o\
		$(STMDDIR)/src/stm32f4xx_syscfg.o\
		$(STMDDIR)/src/stm32f4xx_tim.o\
		$(STMDDIR)/src/stm32f4xx_usart.o\
		$(DISCDIR)/stm32f4_discovery.o\

DEF_STMD=	-DUSE_STDPERIPH_DRIVER\
		-DSTM32F40_41xxx

INC_STMD=	-I$(STMDDIR)/inc\
		-I$(DISCDIR)/

RM_STMD=	$(STMDDIR)/src/*.o\
		$(DISCDIR)/*.o

#######################################

USBHOST=libs/STM32_USB_HOST_Library
USBOTG=libs/STM32_USB_OTG_Driver

OBJ_STOR=	libs/ff/ff.o\
		libs/ff/option/unicode.o\
		$(USBHOST)/Core/src/usbh_core.o\
		$(USBHOST)/Core/src/usbh_hcs.o\
		$(USBHOST)/Core/src/usbh_ioreq.o\
		$(USBHOST)/Core/src/usbh_stdreq.o\
		$(USBHOST)/Class/MSC/src/usbh_msc_bot.o\
		$(USBHOST)/Class/MSC/src/usbh_msc_core.o\
		$(USBHOST)/Class/MSC/src/usbh_msc_fatfs.o\
		$(USBHOST)/Class/MSC/src/usbh_msc_scsi.o\
		$(USBOTG)/src/usb_core.o\
		$(USBOTG)/src/usb_hcd.o\
		$(USBOTG)/src/usb_hcd_int.o\

INC_STOR=	-I$(USBHOST)/Core/inc\
		-I$(USBHOST)/Class/MSC/inc\
		-I$(USBOTG)/inc\
		-Ilibs/ff\

RM_STOR=	$(USBHOST)/Core/src/*.o\
		$(USBHOST)/Class/MSC/src/*.o\
		$(USBOTG)/src/*.o\
		libs/ff/option/*.o\
		libs/ff/*.o\

DEF_STOR=	-DUSE_USB_OTG_FS\
		-DCONFIG_USB\

#######################################

LWIP=libs/lwip-2.0.3

OBJ_LWIP=	$(LWIP)/api/api_lib.o\
		$(LWIP)/api/api_msg.o\
		$(LWIP)/api/err.o\
		$(LWIP)/api/netbuf.o\
		$(LWIP)/api/netdb.o\
		$(LWIP)/api/netifapi.o\
		$(LWIP)/api/sockets.o\
		$(LWIP)/api/tcpip.o\
		$(LWIP)/core/ipv4/autoip.o\
		$(LWIP)/core/ipv4/dhcp.o\
		$(LWIP)/core/ipv4/etharp.o\
		$(LWIP)/core/ipv4/icmp.o\
		$(LWIP)/core/ipv4/igmp.o\
		$(LWIP)/core/ipv4/ip4.o\
		$(LWIP)/core/ipv4/ip4_addr.o\
		$(LWIP)/core/ipv4/ip4_frag.o\
		$(LWIP)/core/def.o\
		$(LWIP)/core/dns.o\
		$(LWIP)/core/inet_chksum.o\
		$(LWIP)/core/init.o\
		$(LWIP)/core/ip.o\
		$(LWIP)/core/mem.o\
		$(LWIP)/core/memp.o\
		$(LWIP)/core/netif.o\
		$(LWIP)/core/pbuf.o\
		$(LWIP)/core/raw.o\
		$(LWIP)/core/stats.o\
		$(LWIP)/core/sys.o\
		$(LWIP)/core/tcp.o\
		$(LWIP)/core/tcp_in.o\
		$(LWIP)/core/tcp_out.o\
		$(LWIP)/core/timeouts.o\
		$(LWIP)/core/udp.o\
		$(LWIP)/netif/ethernet.o\
		$(LWIP)/netif/ethernetif.o\

RM_LWIP=	$(LWIP)/api/*.o\
		$(LWIP)/core/*.o\
		$(LWIP)/core/ipv4/*.o\
		$(LWIP)/netif/*.o\

INC_LWIP=	-I$(LWIP)/include\
		-I$(LWIP)/include/ipv4\
		-I$(LWIP)/\

DEF_LWIP=

CFL_LWIP=

#######################################

LIBMAD=libs/libmad-0.15.1b

OBJ_LIBMAD=	$(LIBMAD)/bit.o\
		$(LIBMAD)/decoder.o\
		$(LIBMAD)/fixed.o\
		$(LIBMAD)/frame.o\
		$(LIBMAD)/huffman.o\
		$(LIBMAD)/layer12.o\
		$(LIBMAD)/layer3.o\
		$(LIBMAD)/stream.o\
		$(LIBMAD)/synth.o\
		$(LIBMAD)/timer.o\
		$(LIBMAD)/version.o

RM_LIBMAD=	$(LIBMAD)/*.o

INC_LIBMAD=	-I$(LIBMAD)

CFL_LIBMAD=	-fforce-addr\
		-fthread-jumps\
		-fcse-follow-jumps\
		-fcse-skip-blocks\
		-fexpensive-optimizations\
		-fregmove\
		-fschedule-insns2\

#######################################

OBJ_STM32=	libs/st/startup_stm32f4xx.o\
		libs/st/system_stm32f4xx.o\
		libs/STM32F4x7_ETH_Driver/src/stm32f4x7_eth.o\
		libs/Audio/Audio.o\
		stm32/allocs.o\
		stm32/stm32f4xx_it.o\
		stm32/console.o\
		stm32/ethernetif.o\
		stm32/fs.o\
		stm32/lcd.o\
		stm32/main.o\
		stm32/net.o\
		stm32/rng.o\
		stm32/sound.o\
		stm32/timer.o\
		stm32/usb.o\
		stm32/usb_bsp.o\

INC_STM32=	-Ilibs/st\
		-Ilibs/cmsis\
		-Ilibs/Audio\
		-Ilibs/STM32F4x7_ETH_Driver/inc\
		-Istm32\
		-I/usr/include\

RM_STM32=	libs/st/*.o\
		stm32/*.o\
		libs/STM32F4x7_ETH_Driver/src/*.o\
		libs/Audio/*.o\

DEF_STM32=	-D__STM32__\
		-D__SALLOC__\
		-DFPM_ARM\
#		-DNO_LCD

CFL_STM32=	-mcpu=cortex-m4\
		-mthumb\
		-nostdlib

LFL_STM32=	-T libs/st/stm32_flash.ld\
		-X\
		-Map=$(TARGET).map

OFL_STM32=	-O binary

#######################################

ifeq ($(PL),stm)

#CROSS=arm-linux-gnu-
#CROSS=arm-none-eabi-

CC=$(CROSS)gcc
LD=$(CROSS)ld
OJ=$(CROSS)objcopy

OBJ=	$(OBJ_STMD)\
	$(OBJ_STOR)\
	$(OBJ_LIBC)\
	$(OBJ_ID3TAG)\
	$(OBJ_LIBMAD)\
	$(OBJ_LWIP)\
	$(OBJ_COMMON)\
	$(OBJ_STM32)\


INC=	$(INC_STMD)\
	$(INC_STOR)\
	$(INC_LIBC)\
	$(INC_ID3TAG)\
	$(INC_LIBMAD)\
	$(INC_LWIP)\
	$(INC_COMMON)\
	$(INC_STM32)

DEF=	$(DEF_STMD)\
	$(DEF_STOR)\
	$(DEF_LIBC)\
	$(DEF_ID3TAG)\
	$(DEF_LIBMAD)\
	$(DEF_LWIP)\
	$(DEF_COMMON)\
	$(DEF_STM32)\

CFL=	$(CFL_STMD)\
	$(CFL_STOR)\
	$(CFL_LIBC)\
	$(CFL_ID3TAG)\
	$(CFL_LIBMAD)\
	$(CFL_LWIP)\
	$(CFL_COMMON)\
	$(CFL_STM32)

LFL=	$(LFL_STMD)\
	$(LFL_STOR)\
	$(LFL_LIBC)\
	$(LFL_ID3TAG)\
	$(LFL_LIBMAD)\
	$(LFL_LWIP)\
	$(LFL_COMMON)\
	$(LFL_STM32)

OFL=	$(OFL_STM32)

endif

#######################################

OBJ_NULL=	null/allocs.o\
		null/console.o\
		null/fs.o\
		null/lcd.o\
		null/main.o\
		null/net.o\
		null/sound.o\
		null/timer.o\

INC_NULL=	-Inull

RM_NULL=	null/*.o

DEF_NULL=	-DFPM_DEFAULT\
		-D__SALLOC__\

CFL_NULL=

LFL_NULL=

#######################################

ifeq ($(PL),null)

CROSS=

CC=$(CROSS)gcc
LD=$(CROSS)gcc
OJ=$(CROSS)objcopy

OBJ=	$(OBJ_LIBC)\
	$(OBJ_ID3TAG)\
	$(OBJ_COMMON)\
	$(OBJ_LIBMAD)\
	$(OBJ_NULL)\

INC=	$(INC_LIBC)\
	$(INC_ID3TAG)\
	$(INC_COMMON)\
	$(INC_LIBMAD)\
	$(INC_NULL)\

DEF=	$(DEF_LIBC)\
	$(DEF_ID3TAG)\
	$(DEF_COMMON)\
	$(DEF_LIBMAD)\
	$(DEF_NULL)\

CFL=	$(CFL_LIBC)\
	$(CFL_ID3TAG)\
	$(CFL_COMMON)\
	$(CFL_LIBMAD)\
	$(CFL_NULL)\

LFL=	$(LFL_LIBC)\
	$(LFL_ID3TAG)\
	$(LFL_COMMON)\
	$(LFL_LIBMAD)\
	$(LFL_NULL)\

endif

#######################################

RM=	\
	$(RM_LIBC)\
	$(RM_ID3TAG)\
	$(RM_COMMON)\
	$(RM_LINUX)\
	$(RM_STM32)\
	$(RM_STMD)\
	$(RM_STOR)\
	$(RM_LIBMAD)\
	$(RM_LWIP)\
	$(RM_STM32)\
	$(RM_NULL)\

CFL+=	$(DEF)\
	$(DEF_DEBUG)

#######################################

.PHONY:	help
.PHONY:	clean
.PHONY:	cleanall
.PHONY:	linux
.PHONY: null
.PHONY: tags
.PHONY: valgrind
.PHONY: callgrind

help:
	@echo	""
	@echo	"Select one of the following targets:"
	@echo	""
	@echo	"make linux		- for builing a linux client"
	@echo   "make stm		- for building a stm32f4 binary"
	@echo	"make clean		- to clean all object files"
	@echo	"make cleanall		- to clean all object files and final binaries"
	@echo	""

linux:
	@$(MAKE) PL=linux target

stm:
	@$(MAKE) PL=stm target

null:
	@$(MAKE) PL=null target

target:	$(OBJ)
	@echo LD $(TARGET).elf
	@$(LD) $(OBJ) -o $(TARGET).elf $(LFL)
	@echo OC $(TARGET).bin
	@$(OJ) $(OFL) $(TARGET).elf $(TARGET).bin

%.o	:%.c
	@echo CC $@
	@$(CC) -c $(CFL) $(INC) $< -o $@

%.o	:%.s
	@echo CC $@
	@$(CC) -c $(CFL) $< -o $@

clean:
	@rm -f $(RM)
	@echo Clean.

cleanall:
	@rm -f $(RM) $(TARGET).elf $(TARGET).bin $(TARGET).map
	@echo All clean.

cleanlinux:
	@rm -f $(RM_LINUX)
	@rm -f $(RM_COMMON)
	@rm -f $(RM_LIBC)
	@rm -f $(RM_ID3TAG)
	@echo Linux clean done.

cleanstm:
	@rm -f $(RM_STM32)
	@rm -f $(RM_COMMON)
	@rm -f $(RM_LIBC)
	@rm -f $(RM_ID3TAG)
	@echo STM clean done.

cleanlwip:
	@rm -f $(RM_LWIP)
	@echo LWIP clean done.

status:
	@$(foreach V,$(sort $(.VARIABLES)),\
	$(if $(filter-out environment% default automatic,\
	$(origin $V)),$(warning $V=$($V) ($(value $V)))))


tags:
	@echo TAGS
	@ctags --exclude=tmp --exclude=.git -R .

flash:
	st-flash write $(TARGET).bin 0x8000000

valgrind:
	valgrind --leak-check=yes --track-origins=yes --leak-check=full --show-leak-kinds=all --workaround-gcc296-bugs=yes ./$(TARGET).elf

callgrind:
	valgrind --tool=callgrind ./$(TARGET).elf

#######################################

