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

// OpenContext.h: Context object for stream driver files
#ifndef OPENCONTEXT_H
#define OPENCONTEXT_H

#include "ceusbkwrapper_common.h"
#include "ptrset.h"

class DeviceContext;
class DevicePtr;
class UsbDevice;
class TransferList;

class OpenContext {
public:
	OpenContext(DeviceContext* Device);
	~OpenContext();
	BOOL Init();
	
	TransferList* GetTransferList();

	DWORD GetDevices(UKWD_USB_DEVICE* lpDevices, DWORD Size);
	BOOL PutDevices(UKWD_USB_DEVICE* lpDevices, DWORD Size);
	BOOL GetDeviceInfo(UKWD_USB_DEVICE DeviceIdentifier, LPUKWD_USB_DEVICE_INFO lpDeviceInfo);
	BOOL StartControlTransfer(LPUKWD_CONTROL_TRANSFER_INFO lpTransferInfo);
	BOOL StartBulkTransfer(LPUKWD_BULK_TRANSFER_INFO lpTransferInfo);
	BOOL CancelTransfer(LPUKWD_CANCEL_TRANSFER_INFO lpCancelInfo);
	BOOL GetConfigDescriptor(LPUKWD_GET_CONFIG_DESC_INFO lpConfigInfo, LPDWORD lpSize);
	BOOL GetActiveConfigValue(UKWD_USB_DEVICE DeviceIdentifier, PUCHAR pConfigurationValue);
	BOOL SetActiveConfigValue(LPUKWD_SET_ACTIVE_CONFIG_VALUE_INFO lpConfigValueInfo);
	BOOL ClaimInterface(LPUKWD_INTERFACE_INFO lpInterfaceInfo);
	BOOL ReleaseInterface(LPUKWD_INTERFACE_INFO lpInterfaceInfo);
	BOOL SetAltSetting(LPUKWD_SET_ALTSETTING_INFO lpSetAltSettingInfo);
	BOOL ClearHalt(LPUKWD_CLEAR_HALT_INFO lpClearHaltInfo);
	BOOL IsPipeHalted(LPUKWD_IS_PIPE_HALTED_INFO lpIsPipeHaltedInfo, LPBOOL halted);
	BOOL ResetDevice(UKWD_USB_DEVICE DeviceIdentifier);
	BOOL ReenumerateDevice(UKWD_USB_DEVICE DeviceIdentifier);
	BOOL IsKernelDriverActiveForInterface(LPUKWD_INTERFACE_INFO lpInterfaceInfo, PBOOL active);
	BOOL AttachKernelDriverForInterface(LPUKWD_INTERFACE_INFO lpInterfaceInfo);
	BOOL DetachKernelDriverForInterface(LPUKWD_INTERFACE_INFO lpInterfaceInfo);
private:
	BOOL PutDevice(UKWD_USB_DEVICE DeviceIdentifier);
	BOOL Validate(DevicePtr& device);
private:
	TransferList* mTransferList;
	DeviceContext* mDevice;
	HANDLE mMutex;
	PtrSet<UsbDevice> mOpenDevices;
};

#endif // OPENCONTEXT_H