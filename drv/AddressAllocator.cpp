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

// AddressAllocator.cpp : Fake address allocator

#include "StdAfx.h"
#include "AddressAllocator.h"

AddressAllocator::AddressAllocator()
: mLastAddress(0)
{
	memset(mBitset, 0, sizeof(mBitset));
}

AddressAllocator::~AddressAllocator()
{
}

BOOL AddressAllocator::Alloc(unsigned char& Address)
{
	int offset;
	for(offset = 0;	offset <= MAX_ADDRESS; ++offset) {
		int a = mLastAddress + offset + 1;
		while (a > MAX_ADDRESS)
			a -= MAX_ADDRESS + 1;
		if (!mBitset[a]) {
			// Found a free address
			mBitset[a] = 1;
			Address = a; 
			break;
		}
	}
	if (offset > MAX_ADDRESS) {
		return FALSE;
	}
	mLastAddress = Address;
	return TRUE;
}

void AddressAllocator::Free(const unsigned char Address)
{
	mBitset[Address] = 0;
}
