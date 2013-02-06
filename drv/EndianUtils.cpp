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

//

#include "StdAfx.h"
#include "EndianUtils.h"
#include "winsock2.h"

static BOOL gLittleEndianCpu;

// FIXME: All this needs checking (does htons(), htonl(), etc, do what we want on big endian systems?)

void EndianUtils::DetectEndian()
{
	UINT16 val = 0x1234;
	gLittleEndianCpu = (htonl(val) != val);
}

INT16 EndianUtils::HostToBus(INT16 aValue)
{
	return gLittleEndianCpu ? aValue : htons(aValue);
}

INT32 EndianUtils::HostToBus(INT32 aValue)
{
	return gLittleEndianCpu ? aValue : htonl(aValue);
}

UINT16 EndianUtils::HostToBus(UINT16 aValue)
{
	return gLittleEndianCpu ? aValue : htons(aValue);
}

UINT32 EndianUtils::HostToBus(UINT32 aValue)
{
	return gLittleEndianCpu ? aValue : htonl(aValue);
}