/* CE USB KWrapper - a USB kernel driver and user-space library
 * Copyright (C) 2012-2013 RealVNC Ltd.
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

// ceusbkwrapper.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "ceusbkwrapper.h"
#include "ceusbkwrapperi.h"
#include "ceusbkwrapper_common.h"
#include "librarydbg.h"

#include <memory>

DBGPARAM dpCurSettings = {
	TEXT("ceusbkwrapper"), {
		TEXT("Errors"),TEXT("Warnings"),
		TEXT("EntryPoints"),TEXT("Functions"),
		TEXT("Undefined"),TEXT("Undefined"),
		TEXT("Undefined"),TEXT("Undefined"),
		TEXT("Undefined"),TEXT("Undefined"),
		TEXT("Undefined"),TEXT("Undefined"),
		TEXT("Undefined"),TEXT("Undefined"),
		TEXT("Undefined"),TEXT("Undefined") },
	DBG_ERROR | DBG_WARNING
};

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ReasonForCall, 
                       LPVOID lpReserved
                     )
{
	BOOL ret = TRUE;
	if (ReasonForCall != DLL_PROCESS_ATTACH) {
		ENTRYPOINT_MSG((
			TEXT("USBKWrapper!DllMain(0x%08x, %d, 0x%08x)\r\n"),
			hModule, ReasonForCall, lpReserved));
	}
	switch (ReasonForCall)
	{
		case DLL_PROCESS_ATTACH:
			// According to MSDN the "HINSTANCE of a DLL is the same as the HMODULE", so this
			// cast is safe.
			DEBUGREGISTER((HMODULE)hModule);
			ENTRYPOINT_MSG((
				TEXT("USBKWrapper!DllMain(0x%08x, %d, 0x%08x)\r\n"),
				hModule, ReasonForCall, lpReserved));

			if (!DisableThreadLibraryCalls((HMODULE)hModule)) {
				WARN_MSG((TEXT("USBKWrapper!DllMain() ")
					TEXT("failed to call DisableThreadLibraryCalls\r\n")));
			}
			break;
		case DLL_PROCESS_DETACH:
        case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
			break;
    }
    return ret;
}

// Driver helper functions

static BOOL IoCtlReadUsbDeviceInfo(HANDLE hDriver, UKW_DEVICE_PRIV& dev)
{
	DWORD written = 0;
	BOOL ret = DeviceIoControl(
		hDriver,
		IOCTL_UKW_GET_DEVICE_INFO,
		&dev.dev, sizeof(dev.dev),
		&dev.info, sizeof(dev.info), &written,
		NULL);
	dev.info.dwCount = written;
	return ret;
}

static BOOL IoCtlReleaseDeviceIdentifiers(HANDLE hDriver, UKWD_USB_DEVICE* identifiers, DWORD Size)
{
	return DeviceIoControl(
		hDriver,
		IOCTL_UKW_PUT_DEVICES, 
		identifiers, Size * sizeof(UKWD_USB_DEVICE),
		NULL, 0, NULL, NULL);
}

static DWORD ConvertUserToKernelFlags(DWORD dwFlags)
{
#define MAP_FLAG(a, b) if ((dwFlags & a) == a) ret |= b;
	DWORD ret = 0;
	MAP_FLAG(UKW_TF_IN_TRANSFER, USB_IN_TRANSFER);
	MAP_FLAG(UKW_TF_OUT_TRANSFER, USB_OUT_TRANSFER);
	MAP_FLAG(UKW_TF_NO_WAIT, USB_NO_WAIT);
	MAP_FLAG(UKW_TF_SHORT_TRANSFER_OK, USB_SHORT_TRANSFER_OK);
	MAP_FLAG(UKW_TF_SEND_TO_DEVICE, USB_SEND_TO_DEVICE);
	MAP_FLAG(UKW_TF_SEND_TO_INTERFACE, USB_SEND_TO_INTERFACE);
	MAP_FLAG(UKW_TF_SEND_TO_ENDPOINT, USB_SEND_TO_ENDPOINT);
	MAP_FLAG(UKW_TF_DONT_BLOCK_FOR_MEM, USB_DONT_BLOCK_FOR_MEM);
	return ret;
}

// Driver API functions

ceusbkwrapper_API const GUID* UkwDriverGUID()
{
	return &ceusbkwrapper_guid;
}

ceusbkwrapper_API HANDLE UkwOpenDriver()
{
	ENTRYPOINT_MSG((
		TEXT("USBKWrapper!UkwOpenDriver()\r\n")));

	HANDLE ret = CreateFile(DRIVER_DEFAULT_DEVICE,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, INVALID_HANDLE_VALUE);

	if (ret != INVALID_HANDLE_VALUE)
		return ret;

	// This activates a new device for each driver, the alternative would be
	// to have this driver always loaded as a 'Builtin' driver, registering
	// as index 0.
	HANDLE hDevice = ActivateDevice(DEVICE_HKEY_LOCATION, 0);
	if (hDevice == INVALID_HANDLE_VALUE) {
		ERROR_MSG((TEXT("USBKWrapper!UkwOpenDriver() failed to activate device: %d\r\n"), GetLastError()));
		return INVALID_HANDLE_VALUE;
	}
	DEVMGR_DEVICE_INFORMATION di;
	di.dwSize = sizeof(di);
	if (!GetDeviceInformationByDeviceHandle(hDevice, &di)) {
		DWORD error = GetLastError();
		ERROR_MSG((TEXT("USBKWrapper!UkwOpenDriver() failed to get device info: %d\r\n"), error));
		DeactivateDevice(hDevice);
		SetLastError(error);
		return INVALID_HANDLE_VALUE;
	}

	if (wcscmp(DRIVER_DEFAULT_DEVICE, di.szLegacyName) == 0) {
		WARN_MSG((TEXT("USBKWrapper!UkwOpenDriver() activated a device matching the default/builtin name\r\n")));
	}

	ret = CreateFile(di.szDeviceName,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, INVALID_HANDLE_VALUE);

	if (ret == INVALID_HANDLE_VALUE) {
		ERROR_MSG((TEXT("USBKWrapper!UkwOpenDriver() failed to create device file: %d\r\n"), GetLastError()));
	}

	return ret;
}

ceusbkwrapper_API BOOL UkwGetDeviceList(
	HANDLE hDriver,
	LPUKW_DEVICE lpList,
	DWORD Size,
	LPDWORD lpActualSize)
{
	ENTRYPOINT_MSG((
		TEXT("USBKWrapper!UkwGetDeviceList(0x%08x, ...)\r\n"),
		hDriver));

	BOOL ret = TRUE;
	if (lpActualSize)
		*lpActualSize = 0;
	if (!lpList)
		return FALSE;
	// Allocate a temporary array for the IOCTL to place its data in.
	UKWD_USB_DEVICE* identifiers = new (std::nothrow) UKWD_USB_DEVICE[Size];
	if (!identifiers) {
		ERROR_MSG((TEXT("USBKWrapper!UkwGetDeviceList() failed to allocate identifier list\r\n")));
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return FALSE;
	}
	// Perform the actual read from the driver
	DWORD bytesWritten = 0;
	ret = DeviceIoControl(hDriver,
		IOCTL_UKW_GET_DEVICES,
		NULL, 0,
		identifiers, Size * sizeof(UKWD_USB_DEVICE),
		&bytesWritten, NULL);
	if (!ret) {
		delete[] identifiers;
		return ret;
	}
	// Validate the returned values
	DWORD deviceCount = bytesWritten / sizeof(UKWD_USB_DEVICE);
	if (bytesWritten % sizeof(UKWD_USB_DEVICE) != 0 ||
		deviceCount > Size) {
		ERROR_MSG((TEXT("USBKWrapper!UkwGetDeviceList() received bad list size: %d\r\n"), bytesWritten));
		delete[] identifiers;
		return ret;
	}
	// Set all the list entries to NULL to make cleanup easier
	memset(lpList, 0, Size * sizeof(LPUKW_DEVICE));

	// Allocate UKW_DEVICE_PRIV for the number of devices
	for(DWORD i = 0; i < deviceCount; ++i) {
		lpList[i] = new (std::nothrow) UKW_DEVICE_PRIV;
		if (!lpList[i]) {
			ret = FALSE;
			break;
		}
		lpList[i]->hDriver = hDriver;
		lpList[i]->dev = identifiers[i];
		if (!IoCtlReadUsbDeviceInfo(hDriver, *lpList[i])) {
			ret = FALSE;
			break;
		}
	}
	
	if (!ret) {
		// Clear up any allocated but unused UKW_DEVICE_PRIV
		for(DWORD i = 0; i < Size; ++i) {
			if (lpList[i])
				delete lpList[i];
		}
		IoCtlReleaseDeviceIdentifiers(hDriver, identifiers, deviceCount);
	}
	if (lpActualSize && ret)
		*lpActualSize = deviceCount;
	delete[] identifiers;
	return ret;
}

ceusbkwrapper_API void UkwReleaseDeviceList(
	HANDLE hDriver,
	LPUKW_DEVICE lpList,
	DWORD Size)
{
	ENTRYPOINT_MSG((
		TEXT("USBKWrapper!UkwReleaseDeviceList(0x%08x, ...)\r\n"),
		hDriver));
	// Release them one at a time. Could try to allocate an array
	// and ask for one big deallocation, but that'd complicate this
	// function by a large amount.
	for(DWORD i = 0; i < Size; ++i) {
		if (!lpList[i])
			continue;
		IoCtlReleaseDeviceIdentifiers(hDriver, &lpList[i]->dev, 1);
		delete lpList[i];
		lpList[i] = NULL;
	}
}

ceusbkwrapper_API void UkwCloseDriver(HANDLE hDriver)
{
	ENTRYPOINT_MSG((
		TEXT("USBKWrapper!UkwCloseDriver(0x%08x)\r\n"),
		hDriver));
	if (hDriver == INVALID_HANDLE_VALUE)
		return;

	DEVMGR_DEVICE_INFORMATION di;
	di.dwSize = sizeof(di);
	if (!GetDeviceInformationByFileHandle(hDriver, &di)) {
		WARN_MSG((TEXT("USBKWrapper!UkwCloseDriver() failed to get device information, driver might stay active: %d\r\n"), GetLastError()));
		di.hDevice = INVALID_HANDLE_VALUE;
	}

	CloseHandle(hDriver);

	if (di.hDevice != INVALID_HANDLE_VALUE) {
		if (wcscmp(DRIVER_DEFAULT_DEVICE, di.szLegacyName) == 0) {
			WARN_MSG((TEXT("USBKWrapper!UkwCloseDriver() not closing device as it matches the default/builtin name\r\n")));
		} else if (!DeactivateDevice(di.hDevice)) {
			WARN_MSG((TEXT("USBKWrapper!UkwCloseDriver() failed to deactivate device , driver might stay active: %d\r\n"), GetLastError()));
		}
	}
}

ceusbkwrapper_API BOOL UkwGetDeviceAddress(
	UKW_DEVICE lpDevice,
	unsigned char* lpBus,
	unsigned char* lpDevAddr,
	unsigned long* lpSessionId
	)
{
	ENTRYPOINT_MSG((
		TEXT("USBKWrapper!UkwGetDeviceAddress(0x%08x, ...)\r\n"),
		lpDevice));

	if (lpBus)
		*lpBus = lpDevice->info.Bus;
	if (lpDevAddr)
		*lpDevAddr = lpDevice->info.Address;
	if (lpSessionId)
		*lpSessionId = lpDevice->info.SessionId;
	return TRUE;
}

ceusbkwrapper_API BOOL WINAPI UkwGetConfig(
	UKW_DEVICE lpDevice,
	PUCHAR pConfig)
{
	ENTRYPOINT_MSG((
		TEXT("USBKWrapper!UkwGetConfig(0x%08x, ...)\r\n"),
		lpDevice));

	DWORD read = -1;
	return DeviceIoControl(
		lpDevice->hDriver,
		IOCTL_UKW_GET_ACTIVE_CONFIG_VALUE,
		&lpDevice->dev, sizeof(lpDevice->dev),
		pConfig, 1, &read, NULL);
}

ceusbkwrapper_API BOOL WINAPI UkwSetConfig(
	UKW_DEVICE lpDevice,
	UCHAR config)
{
	ENTRYPOINT_MSG((
		TEXT("USBKWrapper!UkwSetConfig(0x%08x, %d)\r\n"),
		lpDevice, config));

	UKWD_SET_ACTIVE_CONFIG_VALUE_INFO cvi;
	cvi.dwCount = sizeof(cvi);
	cvi.lpDevice = lpDevice->dev;
	cvi.value = config;
	return DeviceIoControl(
		lpDevice->hDriver,
		IOCTL_UKW_SET_ACTIVE_CONFIG_VALUE,
		&cvi, cvi.dwCount, NULL, 0, NULL, NULL);
}

ceusbkwrapper_API BOOL UkwGetDeviceDescriptor(
	UKW_DEVICE lpDevice,
	LPUKW_DEVICE_DESCRIPTOR lpDeviceDescriptor
	)
{
	ENTRYPOINT_MSG((
		TEXT("USBKWrapper!UkwGetDeviceDescriptor(0x%08x, ...)\r\n"),
		lpDevice));

	if (!lpDeviceDescriptor)
		return TRUE;
	// Could probably get away with a memcpy to copy between the kernel structure
	// and the external API structure, but copying one at a time is safer.
	lpDeviceDescriptor->bLength = lpDevice->info.Descriptor.bLength;
	lpDeviceDescriptor->bDescriptorType = lpDevice->info.Descriptor.bDescriptorType;
	lpDeviceDescriptor->bcdUSB = lpDevice->info.Descriptor.bcdUSB;
	lpDeviceDescriptor->bDeviceClass = lpDevice->info.Descriptor.bDeviceClass;
	lpDeviceDescriptor->bDeviceSubClass = lpDevice->info.Descriptor.bDeviceSubClass;
	lpDeviceDescriptor->bDeviceProtocol = lpDevice->info.Descriptor.bDeviceProtocol;
	lpDeviceDescriptor->bMaxPacketSize0 = lpDevice->info.Descriptor.bMaxPacketSize0;
	lpDeviceDescriptor->idVendor = lpDevice->info.Descriptor.idVendor;
	lpDeviceDescriptor->idProduct = lpDevice->info.Descriptor.idProduct;
	lpDeviceDescriptor->bcdDevice = lpDevice->info.Descriptor.bcdDevice;
	lpDeviceDescriptor->iManufacturer = lpDevice->info.Descriptor.iManufacturer;
	lpDeviceDescriptor->iProduct = lpDevice->info.Descriptor.iProduct;
	lpDeviceDescriptor->iSerialNumber = lpDevice->info.Descriptor.iSerialNumber;
	lpDeviceDescriptor->bNumConfigurations = lpDevice->info.Descriptor.bNumConfigurations;
	return TRUE;
}

ceusbkwrapper_API BOOL WINAPI UkwGetConfigDescriptor(
	UKW_DEVICE lpDevice,
	DWORD dwConfig,
	LPVOID lpBuffer,
	DWORD dwBufferSize,
	LPDWORD lpActualSize
	)
{
	ENTRYPOINT_MSG((
		TEXT("USBKWrapper!UkwGetConfigDescriptor(0x%08x, %d, ...)\r\n"),
		lpDevice, dwConfig));

	UKWD_GET_CONFIG_DESC_INFO info;
	info.dwCount = sizeof(info);
	info.lpDevice = lpDevice->dev;
	info.dwConfigIndex = (dwConfig == UKW_ACTIVE_CONFIGURATION) ?
		UKWD_ACTIVE_CONFIGURATION : dwConfig;
	info.lpDescriptorBuffer = lpBuffer;
	info.dwDescriptorBufferSize = dwBufferSize;

	if (lpActualSize)
		*lpActualSize = 0;

	return DeviceIoControl(
		lpDevice->hDriver,
		IOCTL_UKW_GET_CONFIG_DESC,
		&info, sizeof(info),
		lpActualSize, sizeof(DWORD),
		NULL, NULL);
}

ceusbkwrapper_API BOOL WINAPI UkwClearHaltHost(
	UKW_DEVICE lpDevice,
	UCHAR endpoint)
{
	ENTRYPOINT_MSG((
		TEXT("USBKWrapper!UkwClearHaltHost(0x%08x, %02x)\r\n"),
		lpDevice, endpoint));

	UKWD_ENDPOINT_INFO info;
	info.dwCount = sizeof(info);
	info.lpDevice = lpDevice->dev;
	info.Endpoint = endpoint;
	return DeviceIoControl(
		lpDevice->hDriver,
		IOCTL_UKW_CLEAR_HALT_HOST,
		&info, sizeof(info),
		NULL, NULL, NULL, NULL);
}

ceusbkwrapper_API BOOL WINAPI UkwClearHaltDevice(
	UKW_DEVICE lpDevice,
	UCHAR endpoint)
{
	ENTRYPOINT_MSG((
		TEXT("USBKWrapper!UkwClearHaltDevice(0x%08x, %02x)\r\n"),
		lpDevice, endpoint));

	UKWD_ENDPOINT_INFO info;
	info.dwCount = sizeof(info);
	info.lpDevice = lpDevice->dev;
	info.Endpoint = endpoint;
	return DeviceIoControl(
		lpDevice->hDriver,
		IOCTL_UKW_CLEAR_HALT_DEVICE,
		&info, sizeof(info),
		NULL, NULL, NULL, NULL);
}

ceusbkwrapper_API BOOL WINAPI UkwIsPipeHalted(
	UKW_DEVICE lpDevice,
	UCHAR endpoint,
	LPBOOL lpHalted)
{
	ENTRYPOINT_MSG((
		TEXT("USBKWrapper!UkwIsPipeHalted(0x%08x, %02x, ...)\r\n"),
		lpDevice, endpoint));

	UKWD_ENDPOINT_INFO info;
	info.dwCount = sizeof(info);
	info.lpDevice = lpDevice->dev;
	info.Endpoint = endpoint;
	return DeviceIoControl(
		lpDevice->hDriver,
		IOCTL_UKW_IS_PIPE_HALTED,
		&info, sizeof(info),
		lpHalted, sizeof(*lpHalted),
		NULL, NULL);
}

ceusbkwrapper_API BOOL WINAPI UkwClaimInterface(
	UKW_DEVICE lpDevice,
	DWORD dwInterface
	)
{
	ENTRYPOINT_MSG((
		TEXT("USBKWrapper!UkwClaimInterface(0x%08x, %d)\r\n"),
		lpDevice, dwInterface));

	UKWD_INTERFACE_INFO info;
	info.dwCount = sizeof(info);
	info.lpDevice = lpDevice->dev;
	info.dwInterface = dwInterface;
	return DeviceIoControl(
		lpDevice->hDriver,
		IOCTL_UKW_CLAIM_INTERFACE,
		&info, sizeof(info),
		NULL, NULL, NULL, NULL);
}

ceusbkwrapper_API BOOL WINAPI UkwReleaseInterface(
	UKW_DEVICE lpDevice,
	DWORD dwInterface
	)
{
	ENTRYPOINT_MSG((
		TEXT("USBKWrapper!UkwReleaseInterface(0x%08x, %d)\r\n"),
		lpDevice, dwInterface));

	UKWD_INTERFACE_INFO info;
	info.dwCount = sizeof(info);
	info.lpDevice = lpDevice->dev;
	info.dwInterface = dwInterface;
	return DeviceIoControl(
		lpDevice->hDriver,
		IOCTL_UKW_RELEASE_INTERFACE,
		&info, sizeof(info),
		NULL, NULL, NULL, NULL);
}

ceusbkwrapper_API BOOL WINAPI UkwSetInterfaceAlternateSetting(
	UKW_DEVICE lpDevice,
	DWORD dwInterface,
	DWORD dwAlternateSetting
	)
{
	ENTRYPOINT_MSG((
		TEXT("USBKWrapper!UkwSetInterfaceAlternateSetting(0x%08x, %d, %d)\r\n"),
		lpDevice, dwInterface, dwAlternateSetting));
	
	UKWD_SET_ALTSETTING_INFO info;
	info.dwCount = sizeof(info);
	info.lpDevice = lpDevice->dev;
	info.dwInterface = dwInterface;
	info.dwAlternateSetting = dwAlternateSetting;
	return DeviceIoControl(
		lpDevice->hDriver,
		IOCTL_UKW_SET_ALTSETTING,
		&info, sizeof(info),
		NULL, NULL, NULL, NULL);
}

ceusbkwrapper_API BOOL UkwCancelTransfer(
	UKW_DEVICE lpDevice,
	LPOVERLAPPED lpOverlapped,
	DWORD dwFlags
	)
{
	ENTRYPOINT_MSG((
		TEXT("USBKWrapper!UkwCancelTransfer(0x%08x, 0x%08x, %08x)\r\n"),
		lpDevice, lpOverlapped, dwFlags));

	UKWD_CANCEL_TRANSFER_INFO info;
	info.dwCount = sizeof(info);
	info.lpDevice = lpDevice->dev;
	info.lpOverlapped = lpOverlapped;
	info.dwFlags = ConvertUserToKernelFlags(dwFlags);

	return DeviceIoControl(
		lpDevice->hDriver,
		IOCTL_UKW_CANCEL_TRANSFER,
		&info, sizeof(info),
		NULL, NULL, NULL, NULL);
}

ceusbkwrapper_API BOOL UkwIssueControlTransfer(
	UKW_DEVICE lpDevice,
	DWORD dwFlags,
	LPUKW_CONTROL_HEADER lpHeader,
	LPVOID lpDataBuffer,
	DWORD dwDataBufferSize,
	LPDWORD pBytesTransferred,
	LPOVERLAPPED lpOverlapped
	)
{
	ENTRYPOINT_MSG((
		TEXT("USBKWrapper!UkwIssueControlTransfer(0x%08x, %08x, ...)\r\n"),
		lpDevice, dwFlags));

	UKWD_CONTROL_TRANSFER_INFO info;
	info.dwCount = sizeof(info);
	info.lpDevice = lpDevice->dev;
	info.dwFlags = ConvertUserToKernelFlags(dwFlags);
	info.Header.bmRequestType = lpHeader->bmRequestType;
	info.Header.bRequest = lpHeader->bRequest;
	info.Header.wIndex = lpHeader->wIndex;
	info.Header.wLength = lpHeader->wLength;
	info.Header.wValue = lpHeader->wValue;
	info.lpDataBuffer = lpDataBuffer;
	info.dwDataBufferSize = dwDataBufferSize;
	info.pBytesTransferred = pBytesTransferred;
	info.lpOverlapped = lpOverlapped;
	return DeviceIoControl(
		lpDevice->hDriver,
		IOCTL_UKW_ISSUE_CONTROL_TRANSFER,
		&info, sizeof(info),
		NULL, NULL, NULL, NULL);
}

ceusbkwrapper_API BOOL UkwIssueBulkTransfer(
	UKW_DEVICE lpDevice,
	DWORD dwFlags,
	UCHAR Endpoint,
	LPVOID lpDataBuffer,
	DWORD dwDataBufferSize,
	LPDWORD pBytesTransferred,
	LPOVERLAPPED lpOverlapped
	)
{
	ENTRYPOINT_MSG((
		TEXT("USBKWrapper!UkwIssueBulkTransfer(0x%08x, %08x, %02x, ...)\r\n"),
		lpDevice, dwFlags, Endpoint));

	UKWD_BULK_TRANSFER_INFO info;
	info.dwCount = sizeof(info);
	info.lpDevice = lpDevice->dev;
	info.Endpoint = Endpoint;
	info.dwFlags = ConvertUserToKernelFlags(dwFlags);
	info.lpDataBuffer = lpDataBuffer;
	info.dwDataBufferSize = dwDataBufferSize;
	info.pBytesTransferred = pBytesTransferred;
	info.lpOverlapped = lpOverlapped;
	return DeviceIoControl(
		lpDevice->hDriver,
		IOCTL_UKW_ISSUE_BULK_TRANSFER,
		&info, sizeof(info),
		NULL, NULL, NULL, NULL);
}

ceusbkwrapper_API BOOL WINAPI UkwResetDevice(
	UKW_DEVICE lpDevice)
{
	ENTRYPOINT_MSG((
		TEXT("USBKWrapper!UkwResetDevice(0x%08x)\r\n"),
		lpDevice));

	return DeviceIoControl(
		lpDevice->hDriver,
		IOCTL_UKW_RESET,
		&lpDevice->dev, sizeof(lpDevice->dev), 
		NULL, NULL, NULL, NULL);
}

ceusbkwrapper_API BOOL WINAPI UkwReenumerateDevice(
	UKW_DEVICE lpDevice)
{
	ENTRYPOINT_MSG((
		TEXT("USBKWrapper!UkwReenumerateDevice(0x%08x)\r\n"),
		lpDevice));

	return DeviceIoControl(
		lpDevice->hDriver,
		IOCTL_UKW_REENUMERATE,
		&lpDevice->dev, sizeof(lpDevice->dev), 
		NULL, NULL, NULL, NULL);
}


ceusbkwrapper_API BOOL WINAPI UkwKernelDriverActive(
	UKW_DEVICE lpDevice, DWORD dwInterface, PBOOL active)
{
	ENTRYPOINT_MSG((
		TEXT("USBKWrapper!UkwKernelDriverActive(0x%08x, %d, ...)\r\n"),
		lpDevice, dwInterface));

	UKWD_INTERFACE_INFO info;
	info.dwCount = sizeof(UKWD_INTERFACE_INFO);
	info.dwInterface = dwInterface;
	info.lpDevice = lpDevice->dev;

	return DeviceIoControl(
		lpDevice->hDriver,
		IOCTL_UKW_KERNEL_DRIVER_ACTIVE,
		&info, sizeof(info), active, sizeof(BOOL),
		NULL, NULL);
}

ceusbkwrapper_API BOOL WINAPI UkwAttachKernelDriver(
	UKW_DEVICE lpDevice, DWORD dwInterface)
{
	ENTRYPOINT_MSG((
		TEXT("USBKWrapper!UkwAttachKernelDriver(0x%08x, %d)\r\n"),
		lpDevice, dwInterface));

	UKWD_INTERFACE_INFO info;
	info.dwCount = sizeof(UKWD_INTERFACE_INFO);
	info.dwInterface = dwInterface;
	info.lpDevice = lpDevice->dev;

	return DeviceIoControl(
		lpDevice->hDriver,
		IOCTL_UKW_ATTACH_KERNEL_DRIVER,
		&info, sizeof(info), NULL, NULL,
		NULL, NULL);
}

ceusbkwrapper_API BOOL WINAPI UkwDetachKernelDriver(
	UKW_DEVICE lpDevice, DWORD dwInterface)
{
	ENTRYPOINT_MSG((
		TEXT("USBKWrapper!UkwDetachKernelDriver(0x%08x, %d)\r\n"),
		lpDevice, dwInterface));

	UKWD_INTERFACE_INFO info;
	info.dwCount = sizeof(UKWD_INTERFACE_INFO);
	info.dwInterface = dwInterface;
	info.lpDevice = lpDevice->dev;

	return DeviceIoControl(
		lpDevice->hDriver,
		IOCTL_UKW_DETACH_KERNEL_DRIVER,
		&info, sizeof(info), NULL, NULL,
		NULL, NULL);
}
