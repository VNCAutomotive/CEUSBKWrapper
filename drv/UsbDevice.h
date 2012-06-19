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

// UsbDevice.h : Class representing a USB device

#ifndef USBDEVICE_H
#define USBDEVICE_H

#include "ceusbkwrapper_common.h"

#include "ReadWriteMutex.h"

template <typename T> class UserBuffer;
class InterfaceClaimers;
class Transfer;

typedef struct  {
	UCHAR bConfigurationValue;
	USHORT wTotalLength;
	UCHAR* pDescriptor;
} USBDEVICE_CONFIG_DESCRIPTOR;

class UsbDevice {
public:
	UsbDevice(
		unsigned char Bus,
		unsigned char Address,
		unsigned long SessionId);
	~UsbDevice();

	void Close();
	BOOL Closed() const;

	BOOL Init(USB_HANDLE hDevice,
		LPCUSB_FUNCS lpUsbFuncs,
		LPCUSB_INTERFACE lpInterface);

	BOOL IsSameDevice(USB_HANDLE hDevice);

	UKWD_USB_DEVICE GetIdentifier();

	// The reference counting adjustment should only ever be
	// called by the UsbDeviceList class.
	// Other code should make use of UsbDeviceList::GetDevice()
	// and UsbDeviceList::PutDevice()
	void IncRef();
	DWORD DecRef();

	void ReleaseAllInterfaces(LPVOID Context);
	BOOL ClaimInterface(DWORD dwInterfaceValue, LPVOID Context);
	BOOL ReleaseInterface(DWORD dwInterfaceValue, LPVOID Context);
	BOOL AnyInterfacesClaimed() const;
	BOOL InterfaceClaimed(DWORD dwInterfaceValue, LPVOID Context) const;
	BOOL FindInterface(UCHAR Endpoint, DWORD& dwInterfaceValue);

	unsigned char Bus() const;
	unsigned char Address() const;
	unsigned long SessionId() const;
	BOOL GetDeviceDescriptor(LPUSB_DEVICE_DESCRIPTOR lpDeviceDescriptor);
	BOOL GetActiveConfigDescriptor(UserBuffer<LPVOID>& buffer, LPDWORD lpSize);
	BOOL GetConfigDescriptor(DWORD dwConfigurationIndex, UserBuffer<LPVOID>& buffer, LPDWORD lpSize);
	BOOL GetActiveConfigValue(PUCHAR pConfigurationValue);
	BOOL SetActiveConfigValue(UCHAR pConfigurationValue);
	BOOL SetAltSetting(DWORD dwInterface, DWORD dwAlternateSetting);
	BOOL IsEndpointHalted(DWORD dwInterface, UCHAR Endpoint, BOOL& halted);
	BOOL ClearHalt(DWORD dwInterface, UCHAR Endpoint);

	BOOL GetTransferStatus(
		USB_TRANSFER hTransfer,
		LPDWORD lpdwBytesTransferred,
		LPDWORD lpdwError);
	BOOL GetTransferStatusNoLock(
		USB_TRANSFER hTransfer,
		LPDWORD lpdwBytesTransferred,
		LPDWORD lpdwError);
	BOOL CancelTransfer(USB_TRANSFER hTransfer, DWORD dwFlags);
	BOOL CloseTransfer(USB_TRANSFER hTransfer);

	USB_TRANSFER IssueVendorTransfer(
		Transfer* callback,
		DWORD dwFlags,
		LPCUSB_DEVICE_REQUEST lpControlHeader,
		LPVOID lpvBuffer);

	USB_TRANSFER IssueBulkTransfer(
		Transfer* callback,
		DWORD dwInterface,
		UCHAR Endpoint,
		DWORD dwFlags,
		DWORD dwDataBufferSize,
		LPVOID lpvBuffer);

	BOOL Reset();
	BOOL Reenumerate();

	BOOL IsKernelDriverActiveForInterface(DWORD dwInterface, PBOOL active);
	BOOL AttachKernelDriverForInterface(DWORD dwInterface);
	BOOL DetachKernelDriverForInterface(DWORD dwInterface);
	BOOL AttachKernelDriverForDevice();

private:
	// This should be called by the destructor, or with the close mutex held already by the 
	// calling code
	void DestroyAllConfigDescriptors();
	// All of these should be called with the close mutex held already by the calling code
	BOOL FetchAllConfigDescriptors();
	BOOL CopyConfigDescriptor(DWORD dwIndex, UserBuffer<LPVOID>& buffer, LPDWORD lpSize);
	BOOL AllocateInterfaceClaimers();
	BOOL CheckKernelDriverActiveForInterface(UCHAR ifnum);
	BOOL CheckKernelDriverActiveForDevice();
	USB_PIPE GetPipeForEndpoint(DWORD dwInterface, UCHAR Endpoint);
	BOOL OpenPipes(InterfaceClaimers& Iface);
	void ClosePipes(InterfaceClaimers& Iface);
	void AdvertiseDevice(BOOL isAttached);
private:
	DWORD mRefCount;
	mutable ReadWriteMutex mCloseMutex; // Mutable to allow locking inside const methods
	USB_HANDLE mDevice;
	LPCUSB_FUNCS mUsbFuncs;
	LPCUSB_INTERFACE mUsbInterface;
	BOOL mRegistered;
	const unsigned char mBus, mAddress;
	const unsigned long mSessionId;
	InterfaceClaimers* mInterfaceClaimers;
	DWORD mInterfaceClaimersCount;
	BOOL mKernelDriverAttached;
	USBDEVICE_CONFIG_DESCRIPTOR* mConfigDescriptors;
	UCHAR mNumConfigurations;
};

#endif USBDEVICE_H
