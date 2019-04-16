#include "stm32f4xx.h"
#include "stm32f4xx_conf.h"

#include "libc.h"

#include "platform.h"
#include "stm32.h"

#include "usbh_core.h"
#include "usbh_msc_core.h"

#define	USB_MAX_NAME		32
#define	USB_MAX_SERIAL		32
#define	USB_MAX_MANUFACT	32

#ifdef	CONFIG_USB_DEBUG
#define	usb_debug(...)	xprintf(__VA_ARGS__)
#else
#define	usb_debug(...)
#endif

USB_OTG_CORE_HANDLE	USB_OTG_Core;
USBH_HOST		USB_Host;

static struct {
	int	present;
	int	speed;
	char	name[USB_MAX_NAME];
	char	serial[USB_MAX_SERIAL];
	char	manufact[USB_MAX_MANUFACT];
} ud;

void USBH_USR_Init (void);
void USBH_USR_DeInit (void);
void USBH_USR_DeviceAttached (void);
void USBH_USR_ResetDevice (void);
void USBH_USR_DeviceDisconnected (void);
void USBH_USR_OverCurrentDetected (void);
void USBH_USR_DeviceSpeedDetected (uint8_t DeviceSpeed);
void USBH_USR_Device_DescAvailable (void *DeviceDesc);
void USBH_USR_DeviceAddressAssigned (void);
void USBH_USR_Configuration_DescAvailable (USBH_CfgDesc_TypeDef * cfgDesc,
			USBH_InterfaceDesc_TypeDef *itfDesc,
			USBH_EpDesc_TypeDef *epDesc);
void USBH_USR_Manufacturer_String (void *ManufacturerString);
void USBH_USR_Product_String (void *ProductString);
void USBH_USR_SerialNum_String (void *SerialNumString);
void USBH_USR_EnumerationDone (void);
USBH_USR_Status USBH_USR_UserInput (void);
int USBH_USR_MSC_Application (void);
void USBH_USR_DeviceNotSupported (void);
void USBH_USR_UnrecoveredError (void);

USBH_Usr_cb_TypeDef	USR_Callbacks = {
	USBH_USR_Init,
	USBH_USR_DeInit,
	USBH_USR_DeviceAttached,
	USBH_USR_ResetDevice,
	USBH_USR_DeviceDisconnected,
	USBH_USR_OverCurrentDetected,
	USBH_USR_DeviceSpeedDetected,
	USBH_USR_Device_DescAvailable,
	USBH_USR_DeviceAddressAssigned,
	USBH_USR_Configuration_DescAvailable,
	USBH_USR_Manufacturer_String,
	USBH_USR_Product_String,
	USBH_USR_SerialNum_String,
	USBH_USR_EnumerationDone,
	USBH_USR_UserInput,
	USBH_USR_MSC_Application,
	USBH_USR_DeviceNotSupported,
	USBH_USR_UnrecoveredError
};

void usb_over_current (void) {
	xprintf ("usb overcurrent\n");
	while (1);
}

void usb_init (void) {
	usb_debug ("usb_init: ");

	uc_memset (&ud, 0, sizeof (ud));

	USBH_Init (&USB_OTG_Core, USB_OTG_FS_CORE_ID, &USB_Host, &USBH_MSC_cb, &USR_Callbacks);

	usb_debug ("OK\n");
}

void usb_loop (void) {
	USBH_Process (&USB_OTG_Core, &USB_Host);
}

int usb_ispresent (void) {
	return (ud.present);
}

void usb_device (void) {

	if (ud.present == 1) {
		xprintf ("Present USB device: '%s' '%s' '%s', speed: %d\n",
			ud.manufact,
			ud.name,
			ud.serial,
			ud.speed);
	} else {
		usb_debug ("No USB device present.\n");
	}
}

//////////////// USB Callbacks //////////////////////

void USBH_USR_Init (void) {
	usb_debug ("USBH_USR_Init ");
}

void USBH_USR_DeInit (void) {
	usb_debug ("USBH_USR_DeInit\n");
}

void USBH_USR_DeviceAttached (void) {

	ud.present = 1;

	usb_debug ("USBH_USR_DeviceAttached\n");
}

void USBH_USR_ResetDevice (void) {
	usb_debug ("USBH_USR_ResetDevice\n");
}

void USBH_USR_DeviceDisconnected (void) {

	fs_umount ();

	uc_memset (&ud, 0, sizeof (ud));

	usb_debug ("USBH_USR_DeviceDisconnected\n");
}

void USBH_USR_OverCurrentDetected (void) {
	usb_debug ("USBH_USR_OverCurrentDetected\n");
}

void USBH_USR_DeviceSpeedDetected (uint8_t DeviceSpeed) {

	if (ud.present == 1) {
		ud.speed = DeviceSpeed;
	}

	usb_debug ("USBH_USR_DeviceSpeedDetected %d\n", DeviceSpeed);
}

void USBH_USR_Device_DescAvailable (void *DeviceDesc) {
	usb_debug ("USBH_USR_Device_DescAvailable\n");
}

void USBH_USR_DeviceAddressAssigned (void) {
	usb_debug ("USBH_USR_DeviceAddressAssigned\n");
}

void USBH_USR_Configuration_DescAvailable (USBH_CfgDesc_TypeDef * cfgDesc,
			USBH_InterfaceDesc_TypeDef *itfDesc,
			USBH_EpDesc_TypeDef *epDesc) {

	usb_debug ("USBH_USR_Configuration_DescAvailable\n");
}

void USBH_USR_Manufacturer_String (void *ManufacturerString) {
	int	len;

	if (ud.present == 1) {
		len = uc_strlen (ManufacturerString);
		if (len > USB_MAX_MANUFACT)
			len = USB_MAX_MANUFACT;

		uc_memcpy (&ud.manufact, ManufacturerString, len);
	}

	usb_debug ("USBH_USR_Manufacturer_String '%s'\n", ManufacturerString);
}

void USBH_USR_Product_String (void *ProductString) {
	int	len;

	if (ud.present == 1) {
		len = uc_strlen (ProductString);
		if (len > USB_MAX_NAME)
			len = USB_MAX_NAME;

		uc_memcpy (&ud.name, ProductString, len);
	}

	usb_debug ("USBH_USR_Product_String '%s'\n", ProductString);
}

void USBH_USR_SerialNum_String (void *SerialNumString) {
	int	len;

	if (ud.present == 1) {
		len = uc_strlen (SerialNumString);
		if (len > USB_MAX_SERIAL)
			len = USB_MAX_SERIAL;

		uc_memcpy (&ud.serial, SerialNumString, len);
	}

	usb_debug ("USBH_USR_SerialNum_String '%s'\n", SerialNumString);
}

void USBH_USR_EnumerationDone (void) {
	USB_OTG_BSP_mDelay(500);

	usb_device ();

	usb_debug ("USBH_USR_EnumerationDone\n");
}

USBH_USR_Status USBH_USR_UserInput (void) {
	usb_debug ("USBH_USR_UserInput\n");

	return USBH_USR_RESP_OK;
}

int USBH_USR_MSC_Application (void) {
//	usb_debug ("USBH_USR_MSC_Application\n");

	if (ud.present == 0)
		return 0;

	if (fs_ismounted() == 0) {
		fs_mount ();
	}
	else
		fs_poll_usb ();

	return 0;
}

void USBH_USR_DeviceNotSupported (void) {
	usb_debug ("USBH_USR_DeviceNotSupported\n");
}

void USBH_USR_UnrecoveredError (void) {
	usb_debug ("USBH_USR_UnrecoveredError\n");
}

//////////////// Command line //////////////////////

void cmd_usb (void) {

	usb_device ();

	xprintf ("\n");
}

