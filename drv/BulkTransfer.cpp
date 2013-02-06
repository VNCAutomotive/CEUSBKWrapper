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

// BulkTransfer.cpp : Represents a pending bulk transfer

#include "StdAfx.h"
#include "BulkTransfer.h"
#include "OpenContext.h"
#include "TransferList.h"
#include "UserBuffer.h"
#include "UsbDevice.h"
#include "UsbDeviceList.h"
#include "drvdbg.h"

BulkTransfer::BulkTransfer(
	OpenContext* OpenContext,
	DevicePtr& device,
	DWORD dwInterface,
	LPUKWD_BULK_TRANSFER_INFO lpTransferInfo)
: Transfer(
	OpenContext,
	device,
	lpTransferInfo->dwFlags,
	lpTransferInfo->lpDataBuffer,
	lpTransferInfo->dwDataBufferSize,
	lpTransferInfo->pBytesTransferred,
	lpTransferInfo->lpOverlapped),
	mInterface(dwInterface),
	mTransferInfo(*lpTransferInfo)
{
	TRANSFERLIFETIME_MSG((
		TEXT("USBKWrapperDrv!BulkTransfer:BulkTransfer() created\r\n")));
}

BulkTransfer::~BulkTransfer()
{
	
}

BOOL BulkTransfer::Start()
{
	if (!Transfer::Validate()) {
		return FALSE;
	}
	
	if (mTransferInfo.lpOverlapped)
		// Increment the reference count so that this stays alive
		// until Transfer::TransferComplete() is called.
		mOpenContext->GetTransferList()->GetTransfer(this);

	USB_TRANSFER transfer = mDevicePtr->IssueBulkTransfer(
		mTransferInfo.lpOverlapped ? this : NULL,
		mInterface,
		mTransferInfo.Endpoint,
		mTransferInfo.dwFlags,
		mTransferInfo.dwDataBufferSize,
		mUserBuffer.Ptr());

	SetTransfer(transfer);

	if (!transfer) {
		ERROR_MSG((TEXT("USBKWrapperDrv!BulkTransfer::Start failed to issue transfer: %i\r\n"), GetLastError()));
		if (mTransferInfo.lpOverlapped)
			// Decrement the reference count as Transfer::TransferComplete()
			// will never be called
			mOpenContext->GetTransferList()->PutTransfer(this);
		return FALSE;
	}

	if (!mTransferInfo.lpOverlapped) {
		DWORD bytesTransferred, transferError;
		if (!mDevicePtr->GetTransferStatus(transfer, &bytesTransferred, &transferError)) {
			ERROR_MSG((TEXT("USBKWrapperDrv!BulkTransfer::Start used invalid transfer handle\r\n")));
			SetLastError(ERROR_INVALID_HANDLE);
			return FALSE;
		}
		if (transferError != USB_NO_ERROR) {
			ERROR_MSG((TEXT("USBKWrapperDrv!BulkTransfer::Start transfer failed with USB error %d\r\n"),
				transferError));
		}
		SetLastError(TranslateError(transferError, bytesTransferred, FALSE));
		SetBytesTransferred(bytesTransferred);
		return transferError == USB_NO_ERROR;
	}
	return TRUE;
}
