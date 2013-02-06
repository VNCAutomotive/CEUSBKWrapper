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

// EndianUtils.h : Provides some endian conversion utilities

#ifndef ENDIANUTILS_H
#define ENDIANUTILS_H

namespace EndianUtils {
	// Should only be called once by the DllAttach
	void DetectEndian();

	// Conversion functions
	INT16 HostToBus(INT16 aValue);
	INT32 HostToBus(INT32 aValue);
	UINT16 HostToBus(UINT16 aValue);
	UINT32 HostToBus(UINT32 aValue);
};

#endif //ENDIANUTILS_H
