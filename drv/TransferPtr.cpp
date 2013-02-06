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

// TransferPtr.cpp : RAII wrapper for Transfer.

#include "StdAfx.h"	
#include "TransferPtr.h"
#include "OpenContext.h"
#include "TransferList.h"

TransferPtr::TransferPtr(OpenContext* OpenContext, LPOVERLAPPED lpOverlapped)
: mList(OpenContext->GetTransferList())
, mTransfer(NULL)
{
	mTransfer = mList->GetTransfer(lpOverlapped);
}

TransferPtr::TransferPtr(const TransferPtr& device)
: mList(device.mList)
, mTransfer(NULL)
{
	mTransfer = mList->GetTransfer(device.mTransfer);
}

BOOL TransferPtr::Valid() const
{
	return mTransfer != NULL;
}

Transfer* TransferPtr::operator->()
{
	return mTransfer;
}

Transfer* TransferPtr::Get()
{
	return mTransfer;
}

TransferPtr::~TransferPtr()
{
	if (mTransfer) {
		mList->PutTransfer(mTransfer);
		mTransfer = NULL;
	}
}