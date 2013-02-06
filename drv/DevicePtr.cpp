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

// DevicePtr.cpp : RAII wrapper for UsbDevice.

#include "StdAfx.h"	
#include "DevicePtr.h"
#include "UsbDeviceList.h"

DevicePtr::DevicePtr(UsbDeviceList* lpList, UKWD_USB_DEVICE DeviceIdentifier)
: mList(lpList),
  mDeviceIdentifier(DeviceIdentifier),
  mDevice(NULL)
{
	mDevice = mList->GetDevice(mDeviceIdentifier);
}

DevicePtr::DevicePtr(const DevicePtr& device)
: mList(device.mList),
  mDeviceIdentifier(device.mDeviceIdentifier),
  mDevice(NULL)
{
	mDevice = mList->GetDevice(device.mDevice);
}

BOOL DevicePtr::Valid() const
{
	return mDevice != NULL;
}

UsbDevice* DevicePtr::operator->()
{
	return mDevice;
}

UsbDevice* DevicePtr::Get()
{
	return mDevice;
}

DevicePtr::~DevicePtr()
{
	if (mDevice) {
		mList->PutDevice(mDevice);
		mDevice = NULL;
	}
}