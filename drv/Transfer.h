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

// Transfer.h : Base class for all transfers
#ifndef TRANSFER_H
#define TRANSFER_H

#include "DevicePtr.h"
#include "UserBuffer.h"

class OpenContext;

class Transfer {
public:
	static DWORD TranslateError(DWORD dwUsbError, DWORD dwBytesTransferred, BOOL Cancelled);

	virtual ~Transfer();
	DWORD TransferComplete();
	LPVOID OverlappedUserPtr();
	BOOL Cancel(UKWD_USB_DEVICE device, DWORD dwFlags);
	BOOL Cancel(DWORD dwFlags);

	// The reference counting adjustment should only ever be
	// called by the TransferList class.
	// Other code should make use of TransferList::GetTransfer()
	// and TransferList::PutTransfer()
	void IncRef();
	DWORD DecRef();
protected:
	Transfer(
		OpenContext* OpenContext,
		DevicePtr& device,
		DWORD dwFlags,
		LPVOID lpUserBuffer,
		DWORD dwUserBufferSize,
		LPDWORD lpUserBytesTransferred,
		LPOVERLAPPED lpUserOverlapped);
	
	BOOL Validate();
	void SetBytesTransferred(DWORD bytesTransferred);
	void SetTransfer(USB_TRANSFER transfer);
private:
	void DoTransferCompleted();
private:
	HANDLE mMutex;
	DWORD mRefCount;
	USB_TRANSFER mTransfer;
	BOOL mTransferCompleted;
	BOOL mCancelled;
protected:
	OpenContext* mOpenContext;
	DevicePtr mDevicePtr;
	UserBuffer<LPVOID> mUserBuffer;
	UserBuffer<LPDWORD> mBytesTransferredBuffer;
	OverlappedUserBuffer mOverlappedBuffer;
};

#endif // TRANSFER_H