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

// UsbDeviceList.h : Maintains a thread-safe list of the currently available USB devices

#ifndef USBDEVICELIST_H
#define USBDEVICELIST_H

#include "ptrset.h"

#include "BusAllocator.h"
#include "ceusbkwrapper_common.h"

// Forward declarations
class UsbDevice;

// Interface filter structure
typedef struct {
	BOOL match;   // If true, value must match, else value must not match
	DWORD value;  // A value or USB_NO_INFO if this field should match-all
} INTERFACE_FILTER_FIELD, *PINTERFACE_FILTER_FIELD, *LPINTERFACE_FILTER_FIELD;
typedef const LPINTERFACE_FILTER_FIELD LPCINTERFACE_FILTER_FIELD;

typedef struct {
	// Filter name
	LPWSTR name;

	// Field values
	INTERFACE_FILTER_FIELD bInterfaceClass;
	INTERFACE_FILTER_FIELD bInterfaceSubClass;
	INTERFACE_FILTER_FIELD bInterfaceProtocol;
	INTERFACE_FILTER_FIELD idVendor;
	INTERFACE_FILTER_FIELD idProduct;

	// Filter flags
	BOOL noAttach;

} INTERFACE_FILTER, *PINTERFACE_FILTER, *LPINTERFACE_FILTER;
typedef const LPINTERFACE_FILTER LPCINTERFACE_FILTER;

class UsbDeviceList {
public:
	// Retrieves the singleton instance of this class
	static UsbDeviceList* Get();
	// Creates the singleton instance of this class if it doesn't already exist
	static UsbDeviceList* Create();
	// Destroys the singleton instance of this class, intended to be called from
	// DllMain
	static void DestroySingleton();

	BOOL AttachDevice(
		USB_HANDLE hDevice,
		LPCUSB_FUNCS lpUsbFuncs,
		LPCUSB_INTERFACE lpInterface,
		LPCWSTR szUniqueDriverId,
		LPBOOL fAcceptControl,
		LPCUSB_DRIVER_SETTINGS lpDriverSettings,
		DWORD dwUnused);
	
	UsbDevice* GetDevice(UKWD_USB_DEVICE identifier);
	UsbDevice* GetDevice(UsbDevice* device);
	void PutDevice(UsbDevice* device);
	// Returns all available (not closed) devices
	DWORD GetAvailableDevices(UsbDevice** lpDevices, DWORD Size);
private:
	UsbDeviceList();
	~UsbDeviceList();
	BOOL Init();

	LONG FindNextInterfaceFilterField(LPCWSTR str, DWORD offset, LPDWORD nextOffset, LPINTERFACE_FILTER_FIELD field);

	void LogFilterField(LPCWSTR name, LPCINTERFACE_FILTER_FIELD field);
	void LogFilterFlag(LPCWSTR name, BOOL flag);
	void LogFilter(LPCWSTR message, LPCINTERFACE_FILTER filter);

	// This should be called by destructor or with mMutex held
	void DestroyInterfaceFilters();

	// These should be called with mMutex held
	void FetchInterfaceFilters(LPCUSB_FUNCS lpUsbFuncs, LPCWSTR szUniqueDriverId);
	BOOL MatchFilterField(BOOL doComparison, DWORD value, LPCINTERFACE_FILTER_FIELD field);
	BOOL FindFilterForInterface(LPCUSB_DEVICE lpDevice, LPCUSB_INTERFACE lpInterface, LPDWORD index);
	void AddInterfaceFilter(LPCWSTR name, LPCWSTR value);

private:
	static UsbDeviceList* mSingleton;
private:
	HANDLE mMutex;
	PtrArray<UsbDevice> mDevices;
	BusAllocator mBusAllocator;
	INTERFACE_FILTER* mInterfaceFilters;
	DWORD mNumInterfaceFilters;
};

#endif