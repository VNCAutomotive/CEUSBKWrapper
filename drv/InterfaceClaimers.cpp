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

// InterfaceClaimers.cpp: Maintains a list of contexts which have claimed an interface and the reference count for
// how many times they've claimed it

#include "StdAfx.h"
#include "InterfaceClaimers.h"
#include "drvdbg.h"

#include <stdlib.h>
#include <new>

// Maximum number of spare slots to have before shrinking the array
#define MAXIMUM_SPARE_SPACE 4

InterfaceClaimers::InterfaceClaimers()
: mInterfaceValue(-1)
, mContexts(NULL)
, mContextsCount(0)
, mContextsLength(0)
, mClaimable(FALSE)
, mEndpoints(NULL)
, mEndpointsCount(0)
{

}

InterfaceClaimers::~InterfaceClaimers()
{
	if (mContextsCount > 0) {
		WARN_MSG((TEXT("InterfaceClaimers::~InterfaceClaimers")
				TEXT(" still have %d claimed contexts on interface %d when destroying\r\n"),
				mContextsCount, mInterfaceValue));
	}
	if (mContexts)
		free(mContexts);
	if (mEndpoints)
		delete [] mEndpoints;
}

BOOL InterfaceClaimers::Init(const USB_INTERFACE& iface)
{
	// iface might not be valid for the lifetime of this object, so copy
	// any information needed from it
	mInterfaceValue = iface.Descriptor.bInterfaceNumber;
	mClaimable = TRUE;
	mEndpoints = new (std::nothrow) Endpoints[iface.Descriptor.bNumEndpoints];
	if (mEndpoints == NULL) {
		SetLastError(ERROR_OUTOFMEMORY);
		return FALSE;
	}
	mEndpointsCount = iface.Descriptor.bNumEndpoints;
	// Copy across the values
	for (DWORD i = 0; i < mEndpointsCount; ++i) {
		mEndpoints[i].Address = iface.lpEndpoints[i].Descriptor.bEndpointAddress;
		mEndpoints[i].Pipe = NULL;
	}
	return TRUE;
}

BOOL InterfaceClaimers::AnyClaimed() const
{
	return mContextsCount != 0;
}

DWORD InterfaceClaimers::InterfaceValue() const
{
	return mInterfaceValue;
}

void InterfaceClaimers::SetClaimable(BOOL claimable)
{
	mClaimable = claimable;
}

BOOL InterfaceClaimers::IsClaimable() const
{
	return mClaimable;
}

BOOL InterfaceClaimers::IsClaimed(LPVOID Context) const
{
	for (DWORD i = 0; i < mContextsCount; ++i) {
		if (mContexts[i].Context == Context)
			return mContexts[i].Count > 0;
	}
	return FALSE;
}

// Set IsFirst to TRUE if this is the first claim on the interface
// from any contexts.
BOOL InterfaceClaimers::Claim(LPVOID Context, BOOL& IsFirst)
{
	IsFirst = FALSE;
	// First look to see if it's claimable and already claimed
	if (!IsClaimable()) {
		SetLastError(ERROR_BUSY);
		return FALSE;
	}
	for (DWORD i = 0; i < mContextsCount; ++i) {
		if (mContexts[i].Context == Context) {
			IsFirst = FALSE;
			++mContexts[i].Count;
			return TRUE;
		}
	}
	// Not claimed so allocate some more space
	const DWORD newIdx = mContextsCount;
	if (!ResizeContexts(mContextsCount + 1)) {
		SetLastError(ERROR_OUTOFMEMORY);
		return FALSE;
	}
	mContexts[newIdx].Context = Context;
	mContexts[newIdx].Count = 1;
	IsFirst = (mContextsCount == 0);
	++mContextsCount;
	return TRUE;
}

BOOL InterfaceClaimers::Release(LPVOID Context)
{
	// First look for the context
	for (DWORD i = 0; i < mContextsCount; ++i) {
		if (mContexts[i].Context == Context &&
			mContexts[i].Count > 0) {
			--mContexts[i].Count;
			RemoveEmptyContexts();
			return TRUE;
		}
	}
	// Didn't find the context, this is an error
	SetLastError(ERROR_NOT_FOUND);
	return FALSE;
}

void InterfaceClaimers::ReleaseAll(LPVOID Context)
{
	// Look for the context
	for (DWORD i = 0; i < mContextsCount; ++i) {
		if (mContexts[i].Context == Context) {
			mContexts[i].Count = 0;
			RemoveEmptyContexts();
			return;
		}
	}
}

void InterfaceClaimers::ReleaseAll()
{
	// Release all claims regardless of context
	for (DWORD i = 0; i < mContextsCount; ++i) {
		mContexts[i].Count = 0;
	}
	RemoveEmptyContexts();
}

BOOL InterfaceClaimers::HasEndpoint(UCHAR Endpoint) const
{
	for (DWORD i = 0; i < mEndpointsCount; ++i) {
		if (mEndpoints[i].Address == Endpoint)
			return TRUE;
	}
	return FALSE;
}

DWORD InterfaceClaimers::GetPipeCount() const
{
	return mEndpointsCount;
}

USB_PIPE InterfaceClaimers::GetPipeForEndpoint(UCHAR Endpoint) const
{
	for (DWORD i = 0; i < mEndpointsCount; ++i) {
		if (mEndpoints[i].Address == Endpoint)
			return mEndpoints[i].Pipe;
	}
	return NULL;
}

BOOL InterfaceClaimers::SetPipeForEndpoint(UCHAR Endpoint, USB_PIPE UsbPipe)
{
	for (DWORD i = 0; i < mEndpointsCount; ++i) {
		if (mEndpoints[i].Address == Endpoint) {
			mEndpoints[i].Pipe = UsbPipe;
			return TRUE;
		}
	}
	return FALSE;
}

USB_PIPE InterfaceClaimers::GetPipeForIndex(DWORD Index) const
{
	if (Index >= mEndpointsCount)
		return NULL;
	return mEndpoints[Index].Pipe;
}

BOOL InterfaceClaimers::SetPipeForIndex(DWORD Index, USB_PIPE UsbPipe)
{
	if (Index >= mEndpointsCount)
		return FALSE;
	mEndpoints[Index].Pipe = UsbPipe;
	return TRUE;
}

BOOL InterfaceClaimers::ResizeContexts(DWORD newContextLength)
{
	if (newContextLength == mContextsLength)
		return TRUE;
	if (mContexts == NULL) {
		mContexts = static_cast<ClaimedContext*>(malloc(sizeof(ClaimedContext) * newContextLength));
		if (mContexts == NULL) {
			SetLastError(ERROR_OUTOFMEMORY);
			return FALSE;
		}
		mContextsLength = newContextLength;
		return TRUE;
	} else if (newContextLength == 0) {
		free(mContexts);
		mContexts = NULL;
		return TRUE;
	}
	ClaimedContext* newLocation =
		static_cast<ClaimedContext*>(realloc(mContexts, sizeof(ClaimedContext) * newContextLength));
	if (newLocation == NULL) {
		SetLastError(ERROR_OUTOFMEMORY);
		return FALSE;
	}
	mContexts = newLocation;
	mContextsLength = newContextLength;
	if (mContextsCount > mContextsLength)
		mContextsCount = mContextsLength;
	return TRUE;
}

void InterfaceClaimers::RemoveEmptyContexts()
{
	// Move the contexts down to fill any gaps
	// This isn't as efficient as it could be, but there
	// should only really be a single empty contex and
	// if efficiency here was an issue then a different
	// data structure should be used.
	for (DWORD i = 0; i < mContextsCount; ++i) {
		if (mContexts[i].Count < 1) {
			memmove(mContexts + i, mContexts + i + 1, sizeof(*mContexts) * (mContextsCount - (i + 1)));
			// Move the counter back to the the one just moved down
			--mContextsCount;
			--i;
		}
	}
	// Shrink array if needed
	if (mContextsLength - mContextsCount > MAXIMUM_SPARE_SPACE) {
		ResizeContexts(mContextsCount);
		SetLastError(ERROR_SUCCESS);
	}
}