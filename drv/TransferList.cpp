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

// TransferList.cpp : Contains reference counted transfers

#include "StdAfx.h"
#include "TransferList.h"
#include "Transfer.h"
#include "MutexLocker.h"
#include "drvdbg.h"

TransferList::TransferList()
: mMutex(NULL)
{
}

TransferList::~TransferList()
{
	// The destruction of the transfers held here is a tricky thing.
	// As the OpenContext which owns this TransferList is being destroyed
	// it can be guarenteed that the only references to the transfers
	// must be in the USB driver somewhere, waiting to complete them.
	//
	// The safest thing to do is to go through and cancel them one by one.
	//
	// This is harder than it sounds as they could start to be removed from
	// the transfer list once we start cancelling them. Additionally WinCE
	// makes no guarentees that the completion callback won't occur in the
	// same thread context as the cancel.
	DWORD count = 0;
	if (mMutex)
		WaitForSingleObject(mMutex, INFINITE);
	while (!mTransfers.empty()) {
		Transfer* lpTransfer = *(mTransfers.begin());
		lpTransfer->IncRef();
		if (mMutex)
			ReleaseMutex(mMutex);
		lpTransfer->Cancel(0); // Synchronous cancel
		if (mMutex)
			WaitForSingleObject(mMutex, INFINITE);
		// Should have now been cancelled so lpTransfer is
		// the only reference.
		DWORD refs = lpTransfer->DecRef();
		ASSERT(refs == 0);
		mTransfers.erase(lpTransfer);
		delete lpTransfer;
	}


	PtrArray<Transfer>::const_iterator iter =
		mTransfers.begin();
	while (iter != mTransfers.end()) {
		delete *iter;
		++count;
		++iter;
	}
	if (count > 0) {
		WARN_MSG((TEXT("USBKWrapperDrv!TransferList::~TransferList() - detected %d leaked transfers\r\n"), count));
	}

	if (mMutex)
		CloseHandle(mMutex);
}

BOOL TransferList::Init()
{
	mMutex = CreateMutex(NULL, FALSE, NULL);
	if (mMutex == NULL) {
		ERROR_MSG((TEXT("USBKWrapperDrv!TransferList::Init() - failed to create mutex\r\n")));
		return FALSE;
	}
	return TRUE;
}

BOOL TransferList::RegisterTransfer(Transfer* lpTransfer)
{
	MutexLocker lock(mMutex);
	return mTransfers.insert(lpTransfer);
}

void TransferList::PutTransfer(Transfer* lpTransfer)
{
	MutexLocker lock(mMutex);
	if (lpTransfer) {
		DWORD count = lpTransfer->DecRef();
		if (count <= 0) {
			mTransfers.erase(lpTransfer);
			delete lpTransfer;
		}
	}
}

Transfer* TransferList::GetTransfer(Transfer* lpTransfer)
{
	MutexLocker lock(mMutex);
	lpTransfer->IncRef();
	return lpTransfer;
}

Transfer* TransferList::GetTransfer(LPOVERLAPPED lpOverlapped)
{
	MutexLocker lock(mMutex);
	if (!lpOverlapped)
		return NULL;
	// This could be done in a way which isn't O(n) by using
	// a map, but then there would also need to be the inverse
	// map so that when the transfer is completed the correct map
	// value could be removed.
	//
	// For now the simple linear approach is used of looking at
	// all pending transfers.
	PtrArray<Transfer>::iterator ret = mTransfers.begin();
	while (ret != mTransfers.end() &&
		((*ret == NULL) || ((*ret)->OverlappedUserPtr() != lpOverlapped)))
		++ret;
	if (ret == mTransfers.end()) {
		// Not worth warning about as transfers might have already completed
		// before being cancelled.
		return NULL;
	}
	(*ret)->IncRef();
	return (*ret);
}

