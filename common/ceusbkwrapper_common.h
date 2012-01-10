/* CE USB KWrapper - a USB kernel driver and user-space library
 * Copyright (C) 2012 RealVNC Ltd.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

// ceusbkwrapper_common.h : Common interface definition file
// between ceusbkwrapper and ceusbkwrapperdrv.

#ifndef CEUSBKWRAPPER_COMMON_H
#define CEUSBKWRAPPER_COMMON_H

#include <windev.h>

// {5A4E0F69-48BF-46a6-8124-72174BFB52D4}
#define DEVCLASS_CEUSBKWRAPPER_STRING TEXT("{5A4E0F69-48BF-46a6-8124-72174BFB52D4}")
#define DEVCLASS_CEUSBKWRAPPER_GUID { 0x5a4e0f69, 0x48bf, 0x46a6, { 0x81, 0x24, 0x72, 0x17, 0x4b, 0xfb, 0x52, 0xd4 } }
#define DEVCLASS_CEUSBKWRAPPER_NAME_PREFIX L"ceusbkwrapper"
static const GUID ceusbkwrapper_guid = DEVCLASS_CEUSBKWRAPPER_GUID;

// These constants define the custom IO controls for the 
// stream interface. For more details about custom IO
// control values see:
// http://msdn.microsoft.com/en-us/library/ms904001.aspx
// 0x8000 = start of OEM range, so pick a bit of the way in
#define FILE_DEVICE_USB_KERNEL_WRAPPER 0x9590

// The 2048 = start of OEM range
#define FUNCTION_OEM_START_RANGE 2048
#define USBKWRAPPER_CTL_CODE(f) \
	CTL_CODE(FILE_DEVICE_USB_KERNEL_WRAPPER, \
			 (f + FUNCTION_OEM_START_RANGE), \
			METHOD_BUFFERED, FILE_ANY_ACCESS)

/* Retrieves an array of all currently connected USB devices.
 * The array is a collection of UKWD_USB_DEVICE elements.
 *
 * If there isn't space in the list for all devices then it will
 * only be filled with as many devices as it can hold.
 */
#define IOCTL_UKW_GET_DEVICES								USBKWRAPPER_CTL_CODE(0)
/* Releases references to a list of USB devices, presented as UKW_USB_DEVICE elements. */
#define IOCTL_UKW_PUT_DEVICES								USBKWRAPPER_CTL_CODE(1)
/* Retrieves the device UKWD_USB_DEVICE_INFO for the provided UKWD_USB_DEVICE. */
#define IOCTL_UKW_GET_DEVICE_INFO						USBKWRAPPER_CTL_CODE(2)
/* Issues a Control transfer request using data inside the provided UKWD_CONTROL_TRANSFER_INFO */
#define IOCTL_UKW_ISSUE_CONTROL_TRANSFER		USBKWRAPPER_CTL_CODE(3)
/* Cancels a transfer request using the provide UKWD_CANCEL_TRANSFER_INFO */
#define IOCTL_UKW_CANCEL_TRANSFER						USBKWRAPPER_CTL_CODE(4)
/* Retrieves a config descriptor using the provided UKWD_GET_CONFIG_DESC_INFO. Returns the size in the DWORD in output. */
#define IOCTL_UKW_GET_CONFIG_DESC						USBKWRAPPER_CTL_CODE(5)
/* Retrieves the active USB configuration value for a provided UKWD_USB_DEVICE */
#define IOCTL_UKW_GET_ACTIVE_CONFIG_VALUE		USBKWRAPPER_CTL_CODE(6)
/* Claims the interface specified in the provided UKWD_INTERFACE_INFO */
#define IOCTL_UKW_CLAIM_INTERFACE               USBKWRAPPER_CTL_CODE(7)
/* Releases the interface specified in the provided UKWD_INTERFACE_INFO */
#define IOCTL_UKW_RELEASE_INTERFACE             USBKWRAPPER_CTL_CODE(8)
/* Sets the alternate setting of the interface specified in the provided UKWD_SET_ALTSETTING_INFO */
#define IOCTL_UKW_SET_ALTSETTING                USBKWRAPPER_CTL_CODE(9)
/* Sets the active USB configuration value for a USB device */
#define IOCTL_UKW_SET_ACTIVE_CONFIG_VALUE		USBKWRAPPER_CTL_CODE(10)
/* Clears halt/stall condition of an endpoint in the provided UKWD_CLEAR_HALT_INFO */
#define IOCTL_UKW_CLEAR_HALT                    USBKWRAPPER_CTL_CODE(11)
/* Resets a provided UKWD_USB_DEVICE */
#define IOCTL_UKW_RESET											USBKWRAPPER_CTL_CODE(12)
/* Tests if a kernel driver is active for a provided UKWD_USB_DEVICE */
#define IOCTL_UKW_KERNEL_DRIVER_ACTIVE			USBKWRAPPER_CTL_CODE(13)
/* Attaches a kernel driver to a given interface for a provided UKWD_USB_DEVICE */
#define IOCTL_UKW_ATTACH_KERNEL_DRIVER			USBKWRAPPER_CTL_CODE(14)
/* Detaches a kernel driver from a given interface for a provided UKWD_USB_DEVICE */
#define IOCTL_UKW_DETACH_KERNEL_DRIVER			USBKWRAPPER_CTL_CODE(15)
/* Issues a Bulk transfer request using data inside the provided UKWD_BULK_TRANSFER_INFO */
#define IOCTL_UKW_ISSUE_BULK_TRANSFER				USBKWRAPPER_CTL_CODE(16)
/* Reenumerates a provided UKWD_USB_DEVICE */
#define IOCTL_UKW_REENUMERATE						USBKWRAPPER_CTL_CODE(17)
/* Retrieves if an endpoint is halted (on the host side) using data inside the provided
   UKWD_IS_PIPE_HALTED_INFO */
#define IOCTL_UKW_IS_PIPE_HALTED					USBKWRAPPER_CTL_CODE(18)

// Used as a configuration index when the current active configuration is desired.
#define UKWD_ACTIVE_CONFIGURATION        -1

typedef LPVOID UKWD_USB_DEVICE;

typedef struct _UKWD_USB_DEVICE_INFO {
	DWORD dwCount;
	unsigned char Bus;
	unsigned char Address;
	unsigned long SessionId;
	USB_DEVICE_DESCRIPTOR Descriptor;
} UKWD_USB_DEVICE_INFO, * PUKWD_USB_DEVICE_INFO, * LPUKWD_USB_DEVICE_INFO;

typedef struct _UKWD_CONTROL_TRANSFER_INFO {
	DWORD dwCount;
	UKWD_USB_DEVICE lpDevice;
	DWORD dwFlags; // Using flags from Usbtypes.h
	USB_DEVICE_REQUEST Header;
	LPVOID lpDataBuffer;
	DWORD dwDataBufferSize;
	LPDWORD pBytesTransferred;
	LPOVERLAPPED lpOverlapped;
} UKWD_CONTROL_TRANSFER_INFO, * PUKWD_CONTROL_TRANSFER_INFO, * LPUKWD_CONTROL_TRANSFER_INFO;

typedef struct _UKWD_BULK_TRANSFER_INFO {
	DWORD dwCount;
	UKWD_USB_DEVICE lpDevice;
	UCHAR Endpoint;
	DWORD dwFlags; // Using flags from Usbtypes.h
	LPVOID lpDataBuffer;
	DWORD dwDataBufferSize;
	LPDWORD pBytesTransferred;
	LPOVERLAPPED lpOverlapped;
} UKWD_BULK_TRANSFER_INFO, * PUKWD_BULK_TRANSFER_INFO, * LPUKWD_BULK_TRANSFER_INFO;

typedef struct _UKWD_CANCEL_TRANSFER_INFO {
	DWORD dwCount;
	UKWD_USB_DEVICE lpDevice;
	LPOVERLAPPED lpOverlapped;
	DWORD dwFlags;
} UKWD_CANCEL_TRANSFER_INFO, * PUKWD_CANCEL_TRANSFER_INFO, * LPUKWD_CANCEL_TRANSFER_INFO;

typedef struct _UKWD_GET_CONFIG_DESC_INFO {
	DWORD dwCount;
	UKWD_USB_DEVICE lpDevice;
	DWORD dwConfigIndex; // Can be UKWD_ACTIVE_CONFIGURATION
	LPVOID lpDescriptorBuffer;
	DWORD dwDescriptorBufferSize;
} UKWD_GET_CONFIG_DESC_INFO, * PUKWD_GET_CONFIG_DESC_INFO, * LPUKWD_GET_CONFIG_DESC_INFO;

typedef struct _UKWD_CLEAR_HALT_INFO {
	DWORD dwCount;
	UKWD_USB_DEVICE lpDevice;
	UCHAR Endpoint;
} UKWD_CLEAR_HALT_INFO, * PUKWD_CLEAR_HALT_INFO, * LPUKWD_CLEAR_HALT_INFO;

typedef struct _UKWD_SET_ACTIVE_CONFIG_VALUE_INFO {
	DWORD dwCount;
	UKWD_USB_DEVICE lpDevice;
	UCHAR value;
} UKWD_SET_ACTIVE_CONFIG_VALUE_INFO, * PUKWD_SET_ACTIVE_CONFIG_VALUE_INFO, * LPUKWD_SET_ACTIVE_CONFIG_VALUE_INFO;

typedef struct _UKWD_INTERFACE_INFO {
	DWORD dwCount;
	UKWD_USB_DEVICE lpDevice;
	DWORD dwInterface;
} UKWD_INTERFACE_INFO, * PUKWD_INTERFACE_INFO, * LPUKWD_INTERFACE_INFO;

typedef struct _UKWD_SET_ALTSETTING_INFO {
	DWORD dwCount;
	UKWD_USB_DEVICE lpDevice;
	DWORD dwInterface;
	DWORD dwAlternateSetting;
} UKWD_SET_ALTSETTING_INFO, * PUKWD_SET_ALTSETTING_INFO, * LPUKWD_SET_ALTSETTING_INFO;

typedef struct _UKWD_IS_PIPE_HALTED_INFO {
	DWORD dwCount;
	UKWD_USB_DEVICE lpDevice;
	UCHAR Endpoint;
} UKWD_IS_PIPE_HALTED_INFO, * PUKWD_IS_PIPE_HALTED_INFO, * LPUKWD_IS_PIPE_HALTED_INFO;
#endif // CEUSBKWRAPPER_COMMON_H
