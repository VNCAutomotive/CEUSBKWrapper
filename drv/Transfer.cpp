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

// Transfer.cpp : Base class for all transfers

#include "StdAfx.h"
#include "Transfer.h"
#include "TransferList.h"
#include "OpenContext.h"
#include "UsbDevice.h"
#include "MutexLocker.h"
#include "drvdbg.h"

static DWORD AccessFlagsForUserBuffer(DWORD dwFlags, LPOVERLAPPED lpOverlapped)
{
	return ((dwFlags & USB_IN_TRANSFER) ? UBA_WRITE : UBA_READ)|
		((lpOverlapped || (dwFlags & USB_NO_WAIT)) ? UBA_ASYNC : 0);
}

static DWORD AccessFlagsForBytesTransferredBuffer(DWORD dwFlags, LPOVERLAPPED lpOverlapped)
{
	return UBA_WRITE | ((lpOverlapped || (dwFlags & USB_NO_WAIT)) ? UBA_ASYNC : 0);
}

Transfer::Transfer(
	OpenContext* OpenContext,
	DevicePtr& device,
	DWORD dwFlags,
	LPVOID lpUserBuffer,
	DWORD dwUserBufferSize,
	LPDWORD lpUserBytesTransferred,
	LPOVERLAPPED lpUserOverlapped)
: mMutex()
, mRefCount(1)
, mTransfer(NULL)
, mTransferCompleted(FALSE)
, mCancelled(FALSE)
, mOpenContext(OpenContext)
, mDevicePtr(device)
, mUserBuffer(
	AccessFlagsForUserBuffer(dwFlags, lpUserOverlapped),
	lpUserBuffer, dwUserBufferSize)
, mBytesTransferredBuffer(
	AccessFlagsForBytesTransferredBuffer(dwFlags, lpUserOverlapped),
	lpUserBytesTransferred)
, mOverlappedBuffer(lpUserOverlapped)
{
	mOpenContext->GetTransferList()->RegisterTransfer(this);
	mMutex = CreateMutex(NULL, FALSE, NULL);
}

Transfer::~Transfer()
{
	TRANSFERLIFETIME_MSG((
		TEXT("USBKWrapperDrv!Transfer:~Transfer() mTransfer: %d mRefCount: %d\r\n"),
		mTransfer, mRefCount));
	if (mTransfer != NULL && mDevicePtr.Valid()) {
		if (!mTransferCompleted)
			mDevicePtr->CancelTransfer(mTransfer, 0);
		mDevicePtr->CloseTransfer(mTransfer);
		mTransfer = NULL;
	}
	if (mMutex) {
		CloseHandle(mMutex);
		mMutex = NULL;
	}
}

void Transfer::IncRef()
{
	// This is done without a lock as this should only be called by TransferList
    ++mRefCount;
	TRANSFERLIFETIME_MSG((
		TEXT("USBKWrapperDrv!Transfer::IncRef() mTransfer: %d mRefCount: %d\r\n"),
		mTransfer, mRefCount));
}

DWORD Transfer::DecRef()
{
    // This is done without a lock as this should only be called by TransferList
	--mRefCount;
	TRANSFERLIFETIME_MSG((
		TEXT("USBKWrapperDrv!Transfer::DecRef() mTransfer: %d mRefCount: %d\r\n"),
		mTransfer, mRefCount));
	return mRefCount;
}
	
LPVOID Transfer::OverlappedUserPtr()
{
	return mOverlappedBuffer.UserPtr();
}


BOOL Transfer::Cancel(UKWD_USB_DEVICE device, DWORD dwFlags)
{
	if (!mTransfer || mTransferCompleted || !mDevicePtr.Valid())
		// Device closed or transfer already completed
		return FALSE;
	if (mDevicePtr->GetIdentifier() != device) {
		ERROR_MSG((TEXT("USBKWrapperDrv!Transfer::Cancel() used unmatching device identifiers 0x%08x and 0x%08x\r\n"),
			device, mDevicePtr->GetIdentifier()));
		return FALSE;
	}
	return Cancel(dwFlags);
}

BOOL Transfer::Cancel(DWORD dwFlags)
{
	if (!mTransfer || mTransferCompleted || !mDevicePtr.Valid())
		// Device closed or transfer already completed
		return FALSE;
	mCancelled = TRUE;
	if (!mDevicePtr->CancelTransfer(mTransfer, dwFlags))
		return FALSE;
	return TRUE;
}

BOOL Transfer::Validate()
{
	if (!mMutex) {
		ERROR_MSG((TEXT("USBKWrapperDrv!Transfer::Validate() failed to create mutex\r\n")));
		return FALSE;
	}
	if (!mDevicePtr.Valid()) {
		ERROR_MSG((TEXT("USBKWrapperDrv!Transfer::Validate() failed to find device handle\r\n")));
		mOverlappedBuffer.Abort();
		SetLastError(ERROR_INVALID_HANDLE);
		return FALSE;
	}
	if (!mUserBuffer.Valid() || (!mOverlappedBuffer.Valid() && mOverlappedBuffer.UserPtr())) {
		ERROR_MSG((TEXT("USBKWrapperDrv!Transfer::Validate() failed to map all user memory\r\n")));
		mOverlappedBuffer.Abort();
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return FALSE;
	}
	if (!mBytesTransferredBuffer.Valid()) {
		ERROR_MSG((TEXT("USBKWrapperDrv!Transfer::Validate() failed to map transfer size user memory\r\n")));
		mOverlappedBuffer.Abort();
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return FALSE;
	}
	return TRUE;
}


void Transfer::SetTransfer(USB_TRANSFER transfer)
{
	// It's possible for TransferComplete() to be called before
	// the function call which returns the transfer has completed.
	// To handle this situation this checks for if mTransfer is set.
	BOOL callCompleted = FALSE;
	{
		// As DoTransferCompleted might delete 'this', the mMutex lock can't be hold
		MutexLocker lock(mMutex);
		callCompleted = mTransferCompleted && transfer;
		mTransfer = transfer;
	}
	if (callCompleted)
		DoTransferCompleted();
	// Must return immediately as 'this' might have been deleted by DoTransferCompleted.
	return;
}

DWORD Transfer::TransferComplete()
{
	// It's possible for TransferComplete() to be called before
	// the function call which returns the transfer has completed.
	// To handle this situation this checks for if mTransfer is set.
	BOOL callCompleted = FALSE;
	{
		// As DoTransferCompleted might delete 'this', the mMutex lock can't be hold
		MutexLocker lock(mMutex);
		callCompleted = mTransfer != NULL;
		mTransferCompleted = true;
	}
	if (callCompleted)
		DoTransferCompleted();
	// Must return immediately as 'this' might have been deleted by DoTransferCompleted.
	return 0;
}

void Transfer::DoTransferCompleted()
{
	if (!mTransfer || !mTransferCompleted)
		return;

	DWORD bytesTransferred, transferError, translatedError;
	if (!mDevicePtr->GetTransferStatusNoLock(mTransfer, &bytesTransferred, &transferError)) {
		ERROR_MSG((TEXT("USBKWrapperDrv!Transfer::TransferComplete() used invalid transfer handle\r\n")));
		transferError = USB_NO_ERROR;
		translatedError = ERROR_INVALID_HANDLE;
		bytesTransferred = 0;
	} else {
		translatedError = TranslateError(transferError, bytesTransferred, mCancelled);
	}
	TRANSFERLIFETIME_MSG((
		TEXT("USBKWrapperDrv!Transfer::TransferComplete() completed (error %d, transferred %d, cancelled %d)\r\n"),
		translatedError, bytesTransferred, mCancelled));
	// Need to flush the IO buffer before completing the overlapped buffer
	SetBytesTransferred(bytesTransferred);
	mOverlappedBuffer.Complete(translatedError, bytesTransferred);
	mOpenContext->GetTransferList()->PutTransfer(this);
	// Must return immediately as 'this' might have been deleted when put.
	return;
}

void Transfer::SetBytesTransferred(DWORD bytesTransferred)
{
	LPDWORD ptr = mBytesTransferredBuffer.Ptr();
	if (ptr) {
		*ptr = bytesTransferred;
		mBytesTransferredBuffer.Flush();
	}
}

DWORD Transfer::TranslateError(DWORD dwUsbError, DWORD dwBytesTransferred, BOOL Cancelled)
{
	if (dwBytesTransferred == 0 && Cancelled && dwUsbError == USB_NO_ERROR)
		return ERROR_CANCELLED;

	switch(dwUsbError) {
	case USB_NO_ERROR:
		return ERROR_SUCCESS;
	case USB_STALL_ERROR:
		return ERROR_NOT_SUPPORTED;
	default:
		WARN_MSG((TEXT("USBKWrapperDrv!Transfer::TranslateError translating unexpected error %d\r\n"),
			dwUsbError));
		return ERROR_INTERNAL_ERROR;
	}
	return ERROR_INTERNAL_ERROR;
}