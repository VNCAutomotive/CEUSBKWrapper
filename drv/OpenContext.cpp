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

// OpenContext.cpp: Context object for stream driver files

#include "StdAfx.h"
#include "OpenContext.h"
#include "DeviceContext.h"
#include "UsbDeviceList.h"
#include "UsbDevice.h"
#include "MutexLocker.h"
#include "DevicePtr.h"
#include "TransferList.h"
#include "Transfer.h"
#include "TransferPtr.h"
#include "ControlTransfer.h"
#include "BulkTransfer.h"
#include "drvdbg.h"

#include <new>

OpenContext::OpenContext(DeviceContext* Device)
: mTransferList(NULL), mDevice(Device), mMutex(NULL)
{

}

OpenContext::~OpenContext()
{
	if (mMutex)
		CloseHandle(mMutex);

	delete mTransferList;

	// Release any leaked devices
	DWORD count = 0;
	PtrArray<UsbDevice>::iterator it = mOpenDevices.begin();
	while(it != mOpenDevices.end()) {
		(*it)->ReleaseAllInterfaces(this);
		mDevice->GetDeviceList()->PutDevice(*it);
		++count;
		++it;
	}
	if (count > 0) {
		WARN_MSG((TEXT("USBKWrapperDrv!OpenContext::~OpenContext() - detected %d leaked devices\r\n"), count));
	}
}

BOOL OpenContext::Init()
{
	mMutex = CreateMutex(NULL, FALSE, NULL);
	if (mMutex == NULL) {
		ERROR_MSG((TEXT("USBKWrapperDrv!OpenContext::Init() - failed to create mutex\r\n")));
		return FALSE;
	}
	mTransferList = new (std::nothrow) TransferList();
	if ((!mTransferList) || (!mTransferList->Init())) {
		ERROR_MSG((TEXT("USBKWrapperDrv!OpenContext::Init() - failed to create transfer list\r\n")));
		return FALSE;
	}
	return TRUE;
}


TransferList* OpenContext::GetTransferList()
{
	return mTransferList;
}

// Checks that device is valid and is open
// by this context.
BOOL OpenContext::Validate(DevicePtr& device)
{
	if (!device.Valid())
		return FALSE;
	PtrArray<UsbDevice>::iterator it
		= mOpenDevices.find(device.Get());
	if (it == mOpenDevices.end()) {
	    WARN_MSG((TEXT("USBKWrapperDrv!OpenContext::Validate")
			TEXT(" - failed to find device in this context for identifier 0x%08x\r\n"),
			device->GetIdentifier()));
		return FALSE;
	} 
	return TRUE;
}

DWORD OpenContext::GetDevices(UKWD_USB_DEVICE* lpDevices, DWORD Size)
{
	MutexLocker lock(mMutex);
	UsbDevice** devices = new (std::nothrow) UsbDevice*[Size];
	if (!devices) {
		ERROR_MSG((TEXT("USBKWrapperDrv!OpenContext::GetDevices() - failed to allocate device list, aborting\r\n")));
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return -1;
	}
	DWORD deviceCount = mDevice->GetDeviceList()->GetAvailableDevices(devices, Size);
	DWORD i = 0;
	
	for(i = 0; i < deviceCount; ++i) {
		if (!mOpenDevices.insert(devices[i])) {
			// Failed to add a device to the list, drop the devices and remove the
			// already added devices
			ERROR_MSG((TEXT("USBKWrapperDrv!OpenContext::GetDevices() - failed to remember all devices, aborting\r\n")));
			for(DWORD j = i; j >= 0; --j) {
				mDevice->GetDeviceList()->PutDevice(devices[j]);
				mOpenDevices.erase(devices[i]);
			}
			deviceCount = -1;
			SetLastError(ERROR_NOT_ENOUGH_MEMORY);
			break;
		}
		lpDevices[i] = devices[i]->GetIdentifier();
	}

	delete[] devices;
	return deviceCount;
}

BOOL OpenContext::PutDevices(UKWD_USB_DEVICE* lpDevices, DWORD Size)
{
	MutexLocker lock(mMutex);
	BOOL ret = TRUE;
	for(DWORD i = 0; i < Size; ++i) {
		if (!PutDevice(lpDevices[i])) {
			ERROR_MSG((TEXT("USBKWrapperDrv!OpenContext::PutDevices() - failed to put device 0x%08x\r\n"),
				lpDevices[i]));
			SetLastError(ERROR_INVALID_HANDLE);
			ret = FALSE;
		}
	}
	return ret;
}

BOOL OpenContext::GetDeviceInfo(UKWD_USB_DEVICE DeviceIdentifier, LPUKWD_USB_DEVICE_INFO lpDeviceInfo)
{
	MutexLocker lock(mMutex);
	DevicePtr dev(mDevice->GetDeviceList(), DeviceIdentifier);
	if (!Validate(dev)) {
		ERROR_MSG((TEXT("USBKWrapperDrv!OpenContext::GetDeviceInfo() - failed to validate device 0x%08x\r\n"),
			DeviceIdentifier));
		SetLastError(ERROR_INVALID_HANDLE);
		return FALSE;
	}
// Macro to check if there is enough space for the next member and return
// if there isn't.
#define DI_CHECK_SPACE_OR_RETURN(M) \
	if (offsetof(UKWD_USB_DEVICE_INFO, M) + sizeof(lpDeviceInfo->M) > lpDeviceInfo->dwCount) {\
		lpDeviceInfo->dwCount = offsetof(UKWD_USB_DEVICE_INFO, M); \
		return TRUE; \
	}

	DI_CHECK_SPACE_OR_RETURN(Bus);
	lpDeviceInfo->Bus = dev->Bus();

	DI_CHECK_SPACE_OR_RETURN(Address);
	lpDeviceInfo->Address = dev->Address();

	DI_CHECK_SPACE_OR_RETURN(SessionId);
	lpDeviceInfo->SessionId = dev->SessionId();

	DI_CHECK_SPACE_OR_RETURN(Descriptor);
	if (!dev->GetDeviceDescriptor(&lpDeviceInfo->Descriptor)) {
		ERROR_MSG((TEXT("USBKWrapperDrv!OpenContext::GetDeviceInfo() - ")
			TEXT("failed to retrieve device descriptor 0x%08x\r\n"),
			DeviceIdentifier));
		return FALSE;
	}

	return TRUE;
}

// Called with the mutex lock already held
BOOL OpenContext::PutDevice(UKWD_USB_DEVICE DeviceIdentifier)
{
	MutexLocker lock(mMutex);
	DevicePtr dev (mDevice->GetDeviceList(), DeviceIdentifier);
	if (!Validate(dev)) {
		SetLastError(ERROR_INVALID_HANDLE);
		return FALSE;
	}
	mOpenDevices.erase(dev.Get());
	// Put for the reference in mOpenDevices
	mDevice->GetDeviceList()->PutDevice(dev.Get());
	return TRUE;
}

BOOL OpenContext::StartControlTransfer(LPUKWD_CONTROL_TRANSFER_INFO lpTransferInfo)
{
	MutexLocker lock(mMutex);
	DevicePtr dev (mDevice->GetDeviceList(), lpTransferInfo->lpDevice);
	if (!Validate(dev)) {
		SetLastError(ERROR_INVALID_HANDLE);
		return FALSE;
	}
	ControlTransfer* ct = new (std::nothrow) ControlTransfer(
			this, dev, lpTransferInfo);
	if (!ct) {
		ERROR_MSG((TEXT("USBKWrapperDrv!OpenContext::StartControlTransfer() - failed to create control transfer, aborting\r\n")));
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return FALSE;
	}
	BOOL ret = ct->Start();
	mTransferList->PutTransfer(ct);
	ct = NULL;
	if (!ret) {
		ERROR_MSG((TEXT("USBKWrapperDrv!OpenContext::StartControlTransfer() - failed to start control transfer %d\r\n"), GetLastError()));
		return FALSE;
	}
	// Ownership of ct has passed to the transfer callback
	return TRUE;
}

BOOL OpenContext::StartBulkTransfer(LPUKWD_BULK_TRANSFER_INFO lpTransferInfo)
{
	MutexLocker lock(mMutex);
	DevicePtr dev (mDevice->GetDeviceList(), lpTransferInfo->lpDevice);
	if (!Validate(dev)) {
		SetLastError(ERROR_INVALID_HANDLE);
		return FALSE;
	}

	// Find the interface for this transfer
	DWORD dwInterface;
	if (!dev->FindInterface(lpTransferInfo->Endpoint, dwInterface)) {
		ERROR_MSG((TEXT("USBKWrapperDrv!OpenContext::StartBulkTransfer() - ")
			TEXT("failed to find interface for endpoint %d on device 0x%08x\r\n"),
			lpTransferInfo->Endpoint, lpTransferInfo->lpDevice));
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	// See if it's already been claimed
	if (!dev->InterfaceClaimed(dwInterface, this)) {
		WARN_MSG((TEXT("USBKWrapperDrv!OpenContext::StartBulkTransfer() - ")
			TEXT("using interface %d on device 0x%08x without claiming\r\n"),
			dwInterface, lpTransferInfo->lpDevice));
		if (!dev->ClaimInterface(dwInterface, this)) {
			return FALSE;
		}
	}

	TRANSFERLIFETIME_MSG((TEXT("USBKWrapperDrv!OpenContext::StartBulkTransfer() on ep %x, flag 0x%08x and size %d\r\n"),
		lpTransferInfo->Endpoint, lpTransferInfo->dwFlags, lpTransferInfo->dwDataBufferSize));

	// Construct and start the bulk transfer
	BulkTransfer* bt = new (std::nothrow) BulkTransfer(
			this, dev, dwInterface, lpTransferInfo);
	if (!bt) {
		ERROR_MSG((TEXT("USBKWrapperDrv!OpenContext::StartBulkTransfer() - failed to create bulk transfer, aborting\r\n")));
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return FALSE;
	}
	BOOL ret = bt->Start();
	mTransferList->PutTransfer(bt);
	bt = NULL;
	if (!ret) {
		ERROR_MSG((TEXT("USBKWrapperDrv!OpenContext::StartBulkTransfer() - failed to start bulk transfer %d\r\n"), GetLastError()));
		return FALSE;
	}
	// Ownership of ct has passed to the transfer callback
	return TRUE;
}

BOOL OpenContext::CancelTransfer(LPUKWD_CANCEL_TRANSFER_INFO lpCancelInfo)
{
	MutexLocker lock(mMutex);
	TransferPtr transfer(this, lpCancelInfo->lpOverlapped);
	if (!transfer.Valid()) {
		TRANSFERLIFETIME_MSG((TEXT("USBKWrapperDrv!OpenContext::CancelTransfer() - failed to find transfer to cancel\r\n")));
		return FALSE;
	}
	return transfer->Cancel(lpCancelInfo->lpDevice, lpCancelInfo->dwFlags);
}

BOOL OpenContext::GetConfigDescriptor(LPUKWD_GET_CONFIG_DESC_INFO lpConfigInfo, LPDWORD lpSize)
{
	MutexLocker lock(mMutex);
	DevicePtr dev (mDevice->GetDeviceList(), lpConfigInfo->lpDevice);
	if (!Validate(dev)) {
		SetLastError(ERROR_INVALID_HANDLE);
		return FALSE;
	}
	UserBuffer<LPVOID> descriptorBuf(UBA_WRITE, lpConfigInfo->lpDescriptorBuffer, lpConfigInfo->dwDescriptorBufferSize);
	if (!descriptorBuf.Valid()) {
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return FALSE;
	}
	BOOL status = FALSE;
	if (lpConfigInfo->dwConfigIndex == UKWD_ACTIVE_CONFIGURATION) {
		status = dev->GetActiveConfigDescriptor(descriptorBuf, lpSize);
	} else {
		status = dev->GetConfigDescriptor(lpConfigInfo->dwConfigIndex, descriptorBuf, lpSize);
	}
	if (!status) {
		ERROR_MSG((TEXT("USBKWrapperDrv!OpenContext::GetConfigDescriptor() - ")
			TEXT("failed to retrieve device 0x%08x config descriptor idx %d\r\n"),
			lpConfigInfo->lpDevice, lpConfigInfo->dwConfigIndex));
		return FALSE;
	}
	descriptorBuf.Flush();
	return TRUE;
}

BOOL OpenContext::GetActiveConfigValue(UKWD_USB_DEVICE DeviceIdentifier, PUCHAR pConfigurationValue)
{
	MutexLocker lock(mMutex);
	DevicePtr dev (mDevice->GetDeviceList(), DeviceIdentifier);
	if (!Validate(dev)) {
		SetLastError(ERROR_INVALID_HANDLE);
		return FALSE;
	}
	return dev->GetActiveConfigValue(pConfigurationValue);
}

BOOL OpenContext::SetActiveConfigValue(LPUKWD_SET_ACTIVE_CONFIG_VALUE_INFO lpConfigValueInfo)
{
	MutexLocker lock(mMutex);
	DevicePtr dev (mDevice->GetDeviceList(), lpConfigValueInfo->lpDevice);
	if (!Validate(dev)) {
		SetLastError(ERROR_INVALID_HANDLE);
		return FALSE;
	}

	if (dev->AnyInterfacesClaimed()) {
		ERROR_MSG((TEXT("USBKWrapperDrv!OpenContext::SetActiveConfigValue() - ")
			TEXT("attempting to set configuration on device 0x%08x with claimed interfaces\r\n"),
			lpConfigValueInfo->lpDevice));
		SetLastError(ERROR_BUSY);
		return FALSE;
	}

	/* WARNING: THIS IS NOT SUPPORTED PROPERLY YET!!
	 * The driver has a prototype implementation for IOCTL_UKW_SET_ACTIVE_CONFIG_VALUE,
	 * which manually sends the SET_CONFIGURATION control transfer. This, however, then
	 * causes WinCE to become out-of-sync with the actual device configuration (most likely
	 * leading to all sorts of fun and games!). Currently, the driver therefore only send
	 * the control transfer if the requested configuration is already active.
	 */
	UCHAR cv = 0;
	GetActiveConfigValue(lpConfigValueInfo->lpDevice, &cv);
	if (cv != lpConfigValueInfo->value) {
		SetLastError(ERROR_NOT_SUPPORTED);
		return FALSE;
	}
	return dev->SetActiveConfigValue(lpConfigValueInfo->value);
}

BOOL OpenContext::ClaimInterface(LPUKWD_INTERFACE_INFO lpInterfaceInfo)
{
	MutexLocker lock(mMutex);
	DevicePtr dev (mDevice->GetDeviceList(), lpInterfaceInfo->lpDevice);
	if (!Validate(dev)) {
		SetLastError(ERROR_INVALID_HANDLE);
		return FALSE;
	}

	return dev->ClaimInterface(lpInterfaceInfo->dwInterface, this);
}

BOOL OpenContext::ReleaseInterface(LPUKWD_INTERFACE_INFO lpInterfaceInfo)
{
	MutexLocker lock(mMutex);
	DevicePtr dev (mDevice->GetDeviceList(), lpInterfaceInfo->lpDevice);
	if (!Validate(dev)) {
		SetLastError(ERROR_INVALID_HANDLE);
		return FALSE;
	}

	return dev->ReleaseInterface(lpInterfaceInfo->dwInterface, this);
}

BOOL OpenContext::SetAltSetting(LPUKWD_SET_ALTSETTING_INFO lpSetAltSettingInfo)
{
	MutexLocker lock(mMutex);
	DevicePtr dev (mDevice->GetDeviceList(), lpSetAltSettingInfo->lpDevice);
	if (!Validate(dev)) {
		SetLastError(ERROR_INVALID_HANDLE);
		return FALSE;
	}

	if (!dev->InterfaceClaimed(lpSetAltSettingInfo->dwInterface, this)) {
		WARN_MSG((TEXT("USBKWrapperDrv!OpenContext::SetAltSetting() - ")
			TEXT("using interface %d on device 0x%08x without claiming\r\n"),
			lpSetAltSettingInfo->dwInterface, lpSetAltSettingInfo->lpDevice));
		if (!dev->ClaimInterface(lpSetAltSettingInfo->dwInterface, this)) {
			return FALSE;
		}
	}
	return dev->SetAltSetting(lpSetAltSettingInfo->dwInterface,
						lpSetAltSettingInfo->dwAlternateSetting);
}

BOOL OpenContext::ClearHaltHost(LPUKWD_ENDPOINT_INFO lpEndpointInfo)
{
	MutexLocker lock(mMutex);
	DevicePtr dev (mDevice->GetDeviceList(), lpEndpointInfo->lpDevice);
	if (!Validate(dev)) {
		SetLastError(ERROR_INVALID_HANDLE);
		return FALSE;
	}

	// Find the interface for this device
	DWORD dwInterface;
	if (!dev->FindInterface(lpEndpointInfo->Endpoint, dwInterface)) {
		ERROR_MSG((TEXT("USBKWrapperDrv!OpenContext::ClearHalt() - ")
			TEXT("failed to find interface for endpoint %d on device 0x%08x\r\n"),
			lpEndpointInfo->Endpoint, lpEndpointInfo->lpDevice));
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	// See if it's already been claimed
	if (!dev->InterfaceClaimed(dwInterface, this)) {
		WARN_MSG((TEXT("USBKWrapperDrv!OpenContext::ClearHalt() - ")
			TEXT("using interface %d on device 0x%08x without claiming\r\n"),
			dwInterface, lpEndpointInfo->lpDevice));
		if (!dev->ClaimInterface(dwInterface, this)) {
			return FALSE;
		}
	}

	return dev->ClearHaltHost(dwInterface, lpEndpointInfo->Endpoint);
}
BOOL OpenContext::ClearHaltDevice(LPUKWD_ENDPOINT_INFO lpEndpointInfo)
{
	MutexLocker lock(mMutex);
	DevicePtr dev (mDevice->GetDeviceList(), lpEndpointInfo->lpDevice);
	if (!Validate(dev)) {
		SetLastError(ERROR_INVALID_HANDLE);
		return FALSE;
	}

	// Find the interface for this device
	DWORD dwInterface;
	if (!dev->FindInterface(lpEndpointInfo->Endpoint, dwInterface)) {
		ERROR_MSG((TEXT("USBKWrapperDrv!OpenContext::ClearHalt() - ")
			TEXT("failed to find interface for endpoint %d on device 0x%08x\r\n"),
			lpEndpointInfo->Endpoint, lpEndpointInfo->lpDevice));
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	// See if it's already been claimed
	if (!dev->InterfaceClaimed(dwInterface, this)) {
		WARN_MSG((TEXT("USBKWrapperDrv!OpenContext::ClearHalt() - ")
			TEXT("using interface %d on device 0x%08x without claiming\r\n"),
			dwInterface, lpEndpointInfo->lpDevice));
		if (!dev->ClaimInterface(dwInterface, this)) {
			return FALSE;
		}
	}

	return dev->ClearHaltDevice(dwInterface, lpEndpointInfo->Endpoint);
}

BOOL OpenContext::IsPipeHalted(LPUKWD_ENDPOINT_INFO lpEndpointInfo, LPBOOL halted)
{
	MutexLocker lock(mMutex);
	DevicePtr dev (mDevice->GetDeviceList(), lpEndpointInfo->lpDevice);
	if (!Validate(dev) || halted == NULL) {
		SetLastError(ERROR_INVALID_HANDLE);
		return FALSE;
	}

	// Find the interface for this device
	DWORD dwInterface = -1;
	if (lpEndpointInfo->Endpoint != 0) {
		if (!dev->FindInterface(lpEndpointInfo->Endpoint, dwInterface)) {
			ERROR_MSG((TEXT("USBKWrapperDrv!OpenContext::IsPipeHalted() - ")
				TEXT("failed to find interface for endpoint %d on device 0x%08x\r\n"),
				lpEndpointInfo->Endpoint, lpEndpointInfo->lpDevice));
			SetLastError(ERROR_INVALID_PARAMETER);
			return FALSE;
		}

		// See if it's already been claimed
		if (!dev->InterfaceClaimed(dwInterface, this)) {
			WARN_MSG((TEXT("USBKWrapperDrv!OpenContext::IsPipeHalted() - ")
				TEXT("using interface %d on device 0x%08x without claiming\r\n"),
				dwInterface, lpEndpointInfo->lpDevice));
			if (!dev->ClaimInterface(dwInterface, this)) {
				return FALSE;
			}
		}
	}

	return dev->IsEndpointHalted(dwInterface, lpEndpointInfo->Endpoint, *halted);
}

BOOL OpenContext::ResetDevice(UKWD_USB_DEVICE DeviceIdentifier)
{
	MutexLocker lock(mMutex);
	DevicePtr dev (mDevice->GetDeviceList(), DeviceIdentifier);
	if (!Validate(dev)) {
		SetLastError(ERROR_INVALID_HANDLE);
		return FALSE;
	}
	return dev->Reset();
}

BOOL OpenContext::ReenumerateDevice(UKWD_USB_DEVICE DeviceIdentifier)
{
	MutexLocker lock(mMutex);
	DevicePtr dev (mDevice->GetDeviceList(), DeviceIdentifier);
	if (!Validate(dev)) {
		SetLastError(ERROR_INVALID_HANDLE);
		return FALSE;
	}
	return dev->Reenumerate();
}

BOOL OpenContext::IsKernelDriverActiveForInterface(LPUKWD_INTERFACE_INFO lpInterfaceInfo, PBOOL active)
{
	MutexLocker lock(mMutex);
	DevicePtr dev (mDevice->GetDeviceList(), lpInterfaceInfo->lpDevice);
	if (!Validate(dev)) {
		SetLastError(ERROR_INVALID_HANDLE);
		return FALSE;
	}
	return dev->IsKernelDriverActiveForInterface(lpInterfaceInfo->dwInterface, active);
}

BOOL OpenContext::AttachKernelDriverForInterface(LPUKWD_INTERFACE_INFO lpInterfaceInfo)
{
	MutexLocker lock(mMutex);
	DevicePtr dev (mDevice->GetDeviceList(), lpInterfaceInfo->lpDevice);
	if (!Validate(dev)) {
		SetLastError(ERROR_INVALID_HANDLE);
		return FALSE;
	}
	return dev->AttachKernelDriverForInterface(lpInterfaceInfo->dwInterface);
}

BOOL OpenContext::DetachKernelDriverForInterface(LPUKWD_INTERFACE_INFO lpInterfaceInfo)
{
	MutexLocker lock(mMutex);
	DevicePtr dev (mDevice->GetDeviceList(), lpInterfaceInfo->lpDevice);
	if (!Validate(dev)) {
		SetLastError(ERROR_INVALID_HANDLE);
		return FALSE;
	}
	return dev->DetachKernelDriverForInterface(lpInterfaceInfo->dwInterface);
}
