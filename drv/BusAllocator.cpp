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

// BusAllocator.cpp : Fake bus and address allocator

#include "StdAfx.h"
#include "BusAllocator.h"
#include "AddressAllocator.h"

#include "drvdbg.h"

BusAllocator::BusAllocator()
: mNextSessionId(0)
{
}

BusAllocator::~BusAllocator()
{
}

BOOL BusAllocator::Alloc(
	unsigned char& Bus,
	unsigned char& Address,
	unsigned long& SessionId)
{
	for (int currentBus = 0; currentBus < MAX_BUSES; ++currentBus) {
		if (mAllocators[currentBus].Alloc(Address)) {
			Bus = currentBus;
			SessionId = mNextSessionId++;
			return TRUE;
		}
	}
	return FALSE;
}

void BusAllocator::Free(
	const unsigned char Bus,
	const unsigned char Address,
	const unsigned long SessionId)
{
	// Ignore the session ID
	(void) SessionId;
	mAllocators[Bus].Free(Address);
}