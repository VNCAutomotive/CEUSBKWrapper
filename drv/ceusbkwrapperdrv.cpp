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

// ceusbkwrapperdrv.cpp : Defines the entry points for the DLL application.
// Also defines the dpCurSettings debug zone settings
//

#include "stdafx.h"
#include "ceusbkwrapperdrv.h"
#include "drvdbg.h"
#include "UsbDeviceList.h"
#include "DeviceContext.h"
#include "OpenContext.h"
#include "ceusbkwrapper_common.h"
#include "EndianUtils.h"

#include <new>

DBGPARAM dpCurSettings = {
	TEXT("ceusbkwrapperdrv"), {
		TEXT("Errors"),TEXT("Warnings"),
		TEXT("EntryPoints"),TEXT("Functions"),
		TEXT("DeviceLifetime"),TEXT("TransferLifetime"),
		TEXT("Discovery"),TEXT("InterfaceFilter"),
		TEXT("Undefined"),TEXT("Undefined"),
		TEXT("Undefined"),TEXT("Undefined"),
		TEXT("Undefined"),TEXT("Undefined"),
		TEXT("Undefined"),TEXT("Undefined") },
	DBG_ERROR | DBG_WARNING | DBG_DISCOVERY 
	| DBG_IFACEFILTER
};

static HANDLE gDeviceHandle = INVALID_HANDLE_VALUE;

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ReasonForCall, 
                       LPVOID lpReserved
                     )
{
	BOOL ret = TRUE;
	if (ReasonForCall != DLL_PROCESS_ATTACH) {
		ENTRYPOINT_MSG((
			TEXT("USBKWrapperDrv!DllMain(0x%08x, %d, 0x%08x)\r\n"),
			hModule, ReasonForCall, lpReserved));
	}
	switch (ReasonForCall)
	{
		case DLL_PROCESS_ATTACH:
			EndianUtils::DetectEndian();
			// According to MSDN the "HINSTANCE of a DLL is the same as the HMODULE", so this
			// cast is safe.
			DEBUGREGISTER((HMODULE)hModule);
			ENTRYPOINT_MSG((
				TEXT("USBKWrapperDrv!DllMain(0x%08x, %d, 0x%08x)\r\n"),
				hModule, ReasonForCall, lpReserved));

			if (!DisableThreadLibraryCalls((HMODULE)hModule)) {
				WARN_MSG((TEXT("USBKWrapperDrv!DllMain() ")
					TEXT("failed to call DisableThreadLibraryCalls\r\n")));
			}

			if (!UsbDeviceList::Create()) {
				ERROR_MSG((TEXT("USBKWrapperDrv!DllMain() ")
					TEXT("failed to create UsbDeviceList singleton\r\n")));
				ret = FALSE;
			}
			

			break;
		case DLL_PROCESS_DETACH:
			if (!lpReserved) {
				UsbDeviceList::DestroySingleton();
			}
            break;
        case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
			break;
    }
    return ret;
}

BOOL USBInstallDriver(
	LPCWSTR szDriverLibFile)
{
	ENTRYPOINT_MSG((
		TEXT("USBKWrapperDrv!USBInstallDriver(0x%08x)\r\n"),
		szDriverLibFile));
	return TRUE;
}
BOOL USBDeviceAttach(
	USB_HANDLE hDevice,
	LPCUSB_FUNCS lpUsbFuncs,
	LPCUSB_INTERFACE lpInterface,
	LPCWSTR szUniqueDriverId,
	LPBOOL fAcceptControl,
	LPCUSB_DRIVER_SETTINGS lpDriverSettings,
	DWORD dwUnused)
{
	ENTRYPOINT_MSG((
		TEXT("USBKWrapperDrv!USBDeviceAttach(0x%08x, ...)\r\n"),
		hDevice));
	return UsbDeviceList::Get()->AttachDevice(
		hDevice, lpUsbFuncs, lpInterface, szUniqueDriverId,
		fAcceptControl, lpDriverSettings, dwUnused);
}

BOOL USBUnInstallDriver()
{
	ENTRYPOINT_MSG((
		TEXT("USBKWrapperDrv!USBUnInstallDriver()\r\n")));
	return TRUE;
}

DWORD Init(
  LPCTSTR pContext,
  DWORD dwBusContext)
{
	ENTRYPOINT_MSG((
		TEXT("USBKWrapperDrv!Init(0x%08x, 0x%08x)\r\n"),
		pContext, dwBusContext));
	UsbDeviceList* deviceList = UsbDeviceList::Get();
	if (!deviceList) {
		ERROR_MSG((TEXT("USBKWrapperDrv!Init() failed to get UsbDeviceList\r\n")));
		return 0;
	}
	return reinterpret_cast<DWORD>(new (std::nothrow) DeviceContext(deviceList));
}

BOOL Deinit(
  DWORD hDeviceContext)
{
	ENTRYPOINT_MSG((
		TEXT("USBKWrapperDrv!Deinit(0x%08x)\r\n"),
		hDeviceContext));
	DeviceContext * dev = reinterpret_cast<DeviceContext*>(hDeviceContext);
	delete dev;
	return TRUE;
}

DWORD Open(
  DWORD hDeviceContext,
  DWORD AccessCode,
  DWORD ShareMode)
{
	ENTRYPOINT_MSG((
		TEXT("USBKWrapperDrv!Open(0x%08x, 0x%x, 0x%x)\r\n"),
		hDeviceContext, AccessCode, ShareMode));
	DeviceContext* dev = reinterpret_cast<DeviceContext*>(hDeviceContext);
	OpenContext* ctx = new (std::nothrow) OpenContext(dev);
	if (!ctx || !ctx->Init()) {
		ERROR_MSG((TEXT("USBKWrapperDrv!Open() failed to create open context\r\n")));
		delete ctx;
		ctx = NULL;
	}
	return reinterpret_cast<DWORD>(ctx);
}

BOOL Close(
  DWORD hOpenContext)
{
	ENTRYPOINT_MSG((
		TEXT("USBKWrapperDrv!Close(0x%08x)\r\n"),
		hOpenContext));
	OpenContext* file = reinterpret_cast<OpenContext*>(hOpenContext);
	delete file;
	return TRUE;
}

DWORD Read(
  DWORD hOpenContext,
  LPVOID pBuffer,
  DWORD Count)
{
	ENTRYPOINT_MSG((
		TEXT("USBKWrapperDrv!Read(0x%08x, 0x%08x, %d)\r\n"),
		hOpenContext, pBuffer, Count));
	return -1;
}

DWORD Write(
  DWORD hOpenContext,
  LPVOID pBuffer,
  DWORD Count)
{
	ENTRYPOINT_MSG((
		TEXT("USBKWrapperDrv!Write(0x%08x, 0x%08x, %d)\r\n"),
		hOpenContext, pBuffer, Count));
	return -1;
}

DWORD Seek(
  DWORD hOpenContext,
  long Amount,
  WORD Type)
{
	ENTRYPOINT_MSG((
		TEXT("USBKWrapperDrv!Seek(0x%08x, %l, 0x%x)\r\n"),
		hOpenContext, Amount, Type));
	return -1;
}

BOOL IOControl(
  DWORD hOpenContext,
  DWORD dwCode,
  PBYTE pBufIn,
  DWORD dwLenIn,
  PBYTE pBufOut,
  DWORD dwLenOut,
  PDWORD pdwActualOut)
{
	ENTRYPOINT_MSG((
		TEXT("USBKWrapperDrv!IOControl(0x%08x, 0x%08x (%d), 0x%08x, %d, 0x%08x, %d, ...)\r\n"),
		hOpenContext, dwCode,
		USBKWRAPPER_FUNCTION_FROM_CTL(dwCode), // Extract out the function value for ease
		pBufIn, dwLenIn, pBufOut, dwLenOut));
	OpenContext* file = reinterpret_cast<OpenContext*>(hOpenContext);
	BOOL ret = FALSE;
	switch(dwCode) {
		case IOCTL_UKW_GET_DEVICES:	{
			UKWD_USB_DEVICE* devs = reinterpret_cast<UKWD_USB_DEVICE*>(pBufOut);
			if (dwLenOut % sizeof(UKWD_USB_DEVICE) != 0 || devs == NULL) {
				ERROR_MSG((TEXT("USBKWrapperDrv!IOControl(0x%08x, IOCTL_UKW_GET_DEVICES, ...) ")
					TEXT("passed invalid output len: %d\r\n"), hOpenContext, dwLenOut));
				SetLastError(ERROR_INVALID_PARAMETER);
				break;
			}
			DWORD inCount = dwLenOut / sizeof(UKWD_USB_DEVICE);
			DWORD outCount = file->GetDevices(devs, inCount);
			if (outCount >= 0) {
				if (pdwActualOut)
					*pdwActualOut = outCount * sizeof(UKWD_USB_DEVICE);
				ret = TRUE;
			}
			break;
		}
		case IOCTL_UKW_PUT_DEVICES:	{
			UKWD_USB_DEVICE* devs = reinterpret_cast<UKWD_USB_DEVICE*>(pBufIn);
			if (dwLenIn % sizeof(UKWD_USB_DEVICE) != 0 || devs == NULL) {
				ERROR_MSG((TEXT("USBKWrapperDrv!IOControl(0x%08x, IOCTL_UKW_PUT_DEVICES, ...) ")
					TEXT("passed invalid input len: %d\r\n"), hOpenContext, dwLenIn));
				SetLastError(ERROR_INVALID_PARAMETER);
				break;
			}
			DWORD inCount = dwLenIn / sizeof(UKWD_USB_DEVICE);
			ret = file->PutDevices(devs, inCount);
			break;
		}
		case IOCTL_UKW_GET_DEVICE_INFO:	{
			LPUKWD_USB_DEVICE_INFO di = reinterpret_cast<LPUKWD_USB_DEVICE_INFO>(pBufOut);
			UKWD_USB_DEVICE device = *reinterpret_cast<UKWD_USB_DEVICE*>(pBufIn);
			if (dwLenIn < sizeof(UKWD_USB_DEVICE) || device == NULL) {
				ERROR_MSG((TEXT("USBKWrapperDrv!IOControl(0x%08x, IOCTL_UKW_PUT_DEVICES, ...) ")
					TEXT("passed invalid input len: %d\r\n"), hOpenContext, dwLenIn));
				SetLastError(ERROR_INVALID_PARAMETER);
				break;
			}
			if (dwLenOut < sizeof(di->dwCount) || di == NULL) {
				ERROR_MSG((TEXT("USBKWrapperDrv!IOControl(0x%08x, IOCTL_UKW_PUT_DEVICES, ...) ")
					TEXT("passed invalid output len: %d\r\n"), hOpenContext, dwLenOut));
				SetLastError(ERROR_INVALID_PARAMETER);
				break;
			}
			di->dwCount = dwLenOut;
			ret = file->GetDeviceInfo(device, di);
			if(pdwActualOut)
				*pdwActualOut = ret ? di->dwCount : 0;
			break;
		}
		case IOCTL_UKW_ISSUE_CONTROL_TRANSFER: {
			LPUKWD_CONTROL_TRANSFER_INFO cti = reinterpret_cast<LPUKWD_CONTROL_TRANSFER_INFO>(pBufIn);
			if (dwLenIn < sizeof(LPUKWD_CONTROL_TRANSFER_INFO) ||
				cti == NULL ||
				cti->dwCount < sizeof(LPUKWD_CONTROL_TRANSFER_INFO)) {
				ERROR_MSG((TEXT("USBKWrapperDrv!IOControl(0x%08x, IOCTL_UKW_ISSUE_CONTROL_TRANSFER, ...) ")
					TEXT("passed invalid input len: %d, dwCount %d\r\n"),
					hOpenContext, dwLenIn, (dwLenIn < sizeof(cti->dwCount) && cti) ? 0 : cti->dwCount));
				SetLastError(ERROR_INVALID_PARAMETER);
				break;
			}
			ret = file->StartControlTransfer(cti);
			break;
		}
		case IOCTL_UKW_ISSUE_BULK_TRANSFER: {
			LPUKWD_BULK_TRANSFER_INFO bti = reinterpret_cast<LPUKWD_BULK_TRANSFER_INFO>(pBufIn);
			if (dwLenIn < sizeof(LPUKWD_BULK_TRANSFER_INFO) ||
				bti == NULL ||
				bti->dwCount < sizeof(LPUKWD_BULK_TRANSFER_INFO)) {
				ERROR_MSG((TEXT("USBKWrapperDrv!IOControl(0x%08x, IOCTL_UKW_ISSUE_BULK_TRANSFER, ...) ")
					TEXT("passed invalid input len: %d, dwCount %d\r\n"),
					hOpenContext, dwLenIn, (dwLenIn < sizeof(bti->dwCount) && bti) ? 0 : bti->dwCount));
				SetLastError(ERROR_INVALID_PARAMETER);
				break;
			}
			ret = file->StartBulkTransfer(bti);
			break;
		}
		case IOCTL_UKW_CANCEL_TRANSFER: {
			LPUKWD_CANCEL_TRANSFER_INFO cti = reinterpret_cast<LPUKWD_CANCEL_TRANSFER_INFO>(pBufIn);
			if (dwLenIn < sizeof(LPUKWD_CANCEL_TRANSFER_INFO) ||
				cti == NULL ||
				cti->dwCount < sizeof(LPUKWD_CANCEL_TRANSFER_INFO)) {
				ERROR_MSG((TEXT("USBKWrapperDrv!IOControl(0x%08x, IOCTL_UKW_CANCEL_TRANSFER, ...) ")
					TEXT("passed invalid input len: %d, dwCount %d\r\n"),
					hOpenContext, dwLenIn, (dwLenIn < sizeof(cti->dwCount) && cti) ? 0 : cti->dwCount));
				SetLastError(ERROR_INVALID_PARAMETER);
				break;
			}
			// Clear any unused flags
			cti->dwFlags &= USB_NO_WAIT;
			ret = file->CancelTransfer(cti);
			break;
		}								
		case IOCTL_UKW_GET_CONFIG_DESC:	{
			LPUKWD_GET_CONFIG_DESC_INFO gcdi = reinterpret_cast<LPUKWD_GET_CONFIG_DESC_INFO>(pBufIn);
			LPDWORD ds = reinterpret_cast<LPDWORD>(pBufOut);
			if (dwLenIn < sizeof(UKWD_GET_CONFIG_DESC_INFO) || gcdi == NULL) {
				ERROR_MSG((TEXT("USBKWrapperDrv!IOControl(0x%08x, IOCTL_UKW_GET_CONFIG_DESC, ...) ")
					TEXT("passed invalid input len: %d\r\n"), hOpenContext, dwLenIn));
				SetLastError(ERROR_INVALID_PARAMETER);
				break;
			}
			if (dwLenOut < sizeof(DWORD)) {
				ERROR_MSG((TEXT("USBKWrapperDrv!IOControl(0x%08x, IOCTL_UKW_GET_CONFIG_DESC, ...) ")
					TEXT("passed invalid output len: %d\r\n"), hOpenContext, dwLenOut));
				SetLastError(ERROR_INVALID_PARAMETER);
				break;
			}
			ret = file->GetConfigDescriptor(gcdi, ds);
			if (pdwActualOut)
				*pdwActualOut = (ret && ds != NULL) ? sizeof(DWORD) : 0;
			break;
		}
		case IOCTL_UKW_GET_ACTIVE_CONFIG_VALUE: {
			UKWD_USB_DEVICE * lpDevice = reinterpret_cast<UKWD_USB_DEVICE*>(pBufIn);
			PUCHAR cv = reinterpret_cast<PUCHAR>(pBufOut);
			if (dwLenIn < sizeof(UKWD_USB_DEVICE) || lpDevice == NULL || *lpDevice == NULL) {
				ERROR_MSG((TEXT("USBKWrapperDrv!IOControl(0x%08x, IOCTL_UKW_GET_ACTIVE_CONFIG_VALUE, ...) ")
					TEXT("passed invalid input len: %d\r\n"), hOpenContext, dwLenIn));
				SetLastError(ERROR_INVALID_PARAMETER);
				break;
			}
			if (dwLenOut < sizeof(UCHAR) || cv == NULL) {
				ERROR_MSG((TEXT("USBKWrapperDrv!IOControl(0x%08x, IOCTL_UKW_GET_ACTIVE_CONFIG_VALUE, ...) ")
					TEXT("passed invalid output len: %d\r\n"), hOpenContext, dwLenOut));
				SetLastError(ERROR_INVALID_PARAMETER);
				break;
			}
			UKWD_USB_DEVICE device = *lpDevice;
			ret = file->GetActiveConfigValue(device, cv);
			if (pdwActualOut)
				*pdwActualOut = ret ? 1 : 0;
			break;
		}
		case IOCTL_UKW_SET_ACTIVE_CONFIG_VALUE: {
			LPUKWD_SET_ACTIVE_CONFIG_VALUE_INFO cvi = reinterpret_cast<LPUKWD_SET_ACTIVE_CONFIG_VALUE_INFO>(pBufIn);
			if (dwLenIn < sizeof(UKWD_SET_ACTIVE_CONFIG_VALUE_INFO) ||
				cvi == NULL ||
				cvi->dwCount < sizeof(UKWD_SET_ACTIVE_CONFIG_VALUE_INFO)) {
				ERROR_MSG((TEXT("USBKWrapperDrv!IOControl(0x%08x, IOCTL_UKW_SET_ACTIVE_CONFIG_VALUE, ...) ")
					TEXT("passed invalid input len: %d, dwCount %d\r\n"),
					hOpenContext, dwLenIn, (dwLenIn < sizeof(cvi->dwCount) && cvi) ? 0 : cvi->dwCount));
				SetLastError(ERROR_INVALID_PARAMETER);
				break;
			}
			ret = file->SetActiveConfigValue(cvi);
			break;
		}
		case IOCTL_UKW_CLAIM_INTERFACE: // Deliberate fall through
		case IOCTL_UKW_RELEASE_INTERFACE: {
			LPUKWD_INTERFACE_INFO info = reinterpret_cast<LPUKWD_INTERFACE_INFO>(pBufIn);
			if (dwLenIn < sizeof(UKWD_INTERFACE_INFO) || info == NULL) {
				ERROR_MSG((TEXT("USBKWrapperDrv!IOControl(0x%08x, IOCTL_UKW_CLAIM/RELEASE_INTERFACE, ...) ")
					TEXT("passed invalid input len: %d\r\n"), hOpenContext, dwLenIn));
				SetLastError(ERROR_INVALID_PARAMETER);
				break;
			}
			if (dwCode == IOCTL_UKW_CLAIM_INTERFACE)
				ret = file->ClaimInterface(info);
			else
				ret = file->ReleaseInterface(info);
			break;	
		}
		case IOCTL_UKW_SET_ALTSETTING: {
			LPUKWD_SET_ALTSETTING_INFO info = reinterpret_cast<LPUKWD_SET_ALTSETTING_INFO>(pBufIn);
			if (dwLenIn < sizeof(UKWD_SET_ALTSETTING_INFO) || info == NULL) {
				ERROR_MSG((TEXT("USBKWrapperDrv!IOControl(0x%08x, IOCTL_UKW_SET_ALTSETTING, ...) ")
					TEXT("passed invalid input len: %d\r\n"), hOpenContext, dwLenIn));
				SetLastError(ERROR_INVALID_PARAMETER);
				break;
			}
			ret = file->SetAltSetting(info);
			break;
		}
		case IOCTL_UKW_CLEAR_HALT_HOST: {
			LPUKWD_ENDPOINT_INFO info = reinterpret_cast<LPUKWD_ENDPOINT_INFO>(pBufIn);
			if (dwLenIn < sizeof(LPUKWD_ENDPOINT_INFO) || info == NULL) {
				ERROR_MSG((TEXT("USBKWrapperDrv!IOControl(0x%08x, IOCTL_UKW_CLEAR_HALT, ...) ")
					TEXT("passed invalid input len: %d\r\n"), hOpenContext, dwLenIn));
				SetLastError(ERROR_INVALID_PARAMETER);
				break;
			}
			ret = file->ClearHaltHost(info);
			break;
		}
		case IOCTL_UKW_CLEAR_HALT_DEVICE: {
			LPUKWD_ENDPOINT_INFO info = reinterpret_cast<LPUKWD_ENDPOINT_INFO>(pBufIn);
			if (dwLenIn < sizeof(LPUKWD_ENDPOINT_INFO) || info == NULL) {
				ERROR_MSG((TEXT("USBKWrapperDrv!IOControl(0x%08x, IOCTL_UKW_CLEAR_HALT, ...) ")
					TEXT("passed invalid input len: %d\r\n"), hOpenContext, dwLenIn));
				SetLastError(ERROR_INVALID_PARAMETER);
				break;
			}
			ret = file->ClearHaltDevice(info);
			break;
		}
		case IOCTL_UKW_IS_PIPE_HALTED: {
			LPUKWD_ENDPOINT_INFO info = reinterpret_cast<LPUKWD_ENDPOINT_INFO>(pBufIn);
			LPBOOL ph = reinterpret_cast<LPBOOL>(pBufOut);
			if (dwLenIn < sizeof(LPUKWD_ENDPOINT_INFO) || info == NULL) {
				ERROR_MSG((TEXT("USBKWrapperDrv!IOControl(0x%08x, IOCTL_UKW_IS_PIPE_HALTED, ...) ")
					TEXT("passed invalid input len: %d\r\n"), hOpenContext, dwLenIn));
				SetLastError(ERROR_INVALID_PARAMETER);
				break;
			}
			if (dwLenOut < sizeof(BOOL) || ph == NULL) {
				ERROR_MSG((TEXT("USBKWrapperDrv!IOControl(0x%08x, LPUKWD_IS_PIPE_HALTED_INFO, ...) ")
					TEXT("passed invalid output len: %d\r\n"), hOpenContext, dwLenOut));
				SetLastError(ERROR_INVALID_PARAMETER);
				break;
			}
			ret = file->IsPipeHalted(info, ph);
			if (pdwActualOut)
				*pdwActualOut = ret ? sizeof(BOOL) : 0;
			break;
		}
		case IOCTL_UKW_RESET: {
			UKWD_USB_DEVICE * lpDevice = reinterpret_cast<UKWD_USB_DEVICE*>(pBufIn);
			if (dwLenIn < sizeof(UKWD_USB_DEVICE) || lpDevice == NULL || *lpDevice == NULL) {
				ERROR_MSG((TEXT("USBKWrapperDrv!IOControl(0x%08x, IOCTL_UKW_RESET, ...) ")
					TEXT("passed invalid input len: %d\r\n"), hOpenContext, dwLenIn));
				SetLastError(ERROR_INVALID_PARAMETER);
				break;
			}
			UKWD_USB_DEVICE device = *lpDevice;
			ret = file->ResetDevice(device);
			break;
		}
		case IOCTL_UKW_REENUMERATE: {
			UKWD_USB_DEVICE * lpDevice = reinterpret_cast<UKWD_USB_DEVICE*>(pBufIn);
			if (dwLenIn < sizeof(UKWD_USB_DEVICE) || lpDevice == NULL || *lpDevice == NULL) {
				ERROR_MSG((TEXT("USBKWrapperDrv!IOControl(0x%08x, IOCTL_UKW_REENUMERATE, ...) ")
					TEXT("passed invalid input len: %d\r\n"), hOpenContext, dwLenIn));
				SetLastError(ERROR_INVALID_PARAMETER);
				break;
			}
			UKWD_USB_DEVICE device = *lpDevice;
			ret = file->ReenumerateDevice(device);
			break;
		}
		case IOCTL_UKW_KERNEL_DRIVER_ACTIVE: {
			LPUKWD_INTERFACE_INFO lpInterfaceInfo = reinterpret_cast<LPUKWD_INTERFACE_INFO>(pBufIn);
			PBOOL active = reinterpret_cast<PBOOL>(pBufOut);
			if (dwLenIn < sizeof(UKWD_INTERFACE_INFO) || lpInterfaceInfo == NULL || 
				lpInterfaceInfo->dwCount != sizeof(UKWD_INTERFACE_INFO)) {
				ERROR_MSG((TEXT("USBKWrapperDrv!IOControl(0x%08x, IOCTL_UKW_KERNEL_DRIVER_ACTIVE, ...) ")
					TEXT("passed invalid input len: %d\r\n"), hOpenContext, dwLenIn));
				SetLastError(ERROR_INVALID_PARAMETER);
				break;
			}
			if (dwLenOut < sizeof(BOOL) || active == NULL) {
				ERROR_MSG((TEXT("USBKWrapperDrv!IOControl(0x%08x, IOCTL_UKW_KERNEL_DRIVER_ACTIVE, ...) ")
					TEXT("passed invalid output len: %d\r\n"), hOpenContext, dwLenOut));
				SetLastError(ERROR_INVALID_PARAMETER);
				break;
			}
			ret = file->IsKernelDriverActiveForInterface(lpInterfaceInfo, active);
			break;
		}			
		case IOCTL_UKW_ATTACH_KERNEL_DRIVER: {
			LPUKWD_INTERFACE_INFO lpInterfaceInfo = reinterpret_cast<LPUKWD_INTERFACE_INFO>(pBufIn);
			if (dwLenIn < sizeof(UKWD_INTERFACE_INFO) || lpInterfaceInfo == NULL ||
				lpInterfaceInfo->dwCount != sizeof(UKWD_INTERFACE_INFO)) {
				ERROR_MSG((TEXT("USBKWrapperDrv!IOControl(0x%08x, IOCTL_UKW_ATTACH_KERNEL_DRIVER, ...) ")
					TEXT("passed invalid input len: %d\r\n"), hOpenContext, dwLenIn));
				SetLastError(ERROR_INVALID_PARAMETER);
				break;
			}
			ret = file->AttachKernelDriverForInterface(lpInterfaceInfo);
			break;
		}	
		case IOCTL_UKW_DETACH_KERNEL_DRIVER: {
			LPUKWD_INTERFACE_INFO lpInterfaceInfo = reinterpret_cast<LPUKWD_INTERFACE_INFO>(pBufIn);
			if (dwLenIn < sizeof(UKWD_INTERFACE_INFO) || lpInterfaceInfo == NULL ||
				lpInterfaceInfo->dwCount != sizeof(UKWD_INTERFACE_INFO)) {
				ERROR_MSG((TEXT("USBKWrapperDrv!IOControl(0x%08x, IOCTL_UKW_DETACH_KERNEL_DRIVER, ...) ")
					TEXT("passed invalid input len: %d\r\n"), hOpenContext, dwLenIn));
				SetLastError(ERROR_INVALID_PARAMETER);
				break;
			}
			ret = file->DetachKernelDriverForInterface(lpInterfaceInfo);
			break;
		}	
		default: {
			SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
			break;
		}
	}
	
	return ret;
}

void PowerUp(
  DWORD hDeviceContext)
{
	ENTRYPOINT_MSG((
		TEXT("USBKWrapperDrv!PowerUp(0x%08x)\r\n"),
		hDeviceContext));
}

void PowerDown(
  DWORD hDeviceContext)
{
	ENTRYPOINT_MSG((
		TEXT("USBKWrapperDrv!PowerDown(0x%08x)\r\n"),
		hDeviceContext));
}


BOOL PreClose(
  DWORD hOpenContext)
{
	ENTRYPOINT_MSG((
		TEXT("USBKWrapperDrv!PreClose(0x%08x)\r\n"),
		hOpenContext));
	return TRUE;
}

BOOL PreDeinit(
  DWORD hDeviceContext)
{
	ENTRYPOINT_MSG((
		TEXT("USBKWrapperDrv!PreDeinit(0x%08x)\r\n"),
		hDeviceContext));
	return TRUE;
}