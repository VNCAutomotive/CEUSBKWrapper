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

// ceusbkwrapper.h: Internal implementation details of 
// structs used in the library interface for user-side 
// library for kernel USB wrapper.

#ifndef CEUSBKWRAPPERI_H
#define CEUSBKWRAPPERI_H

#include "ceusbkwrapper_common.h"

// This needs to match the value in ceusbkwrapperdrv.reg
#define DEVICE_HKEY_LOCATION TEXT("Drivers\\USB\\ClientDrivers\\Usb_Kernel_Wrapper")
// Used to allow the driver to be registered as a Builtin driver, so always loaded
#define DRIVER_DEFAULT_DEVICE TEXT("UKW0:")

// Holds information about a device.
struct UKW_DEVICE_PRIV {
	HANDLE hDriver;
	UKWD_USB_DEVICE dev;
	UKWD_USB_DEVICE_INFO info;
};

#endif // CEUSBKWRAPPERI_H