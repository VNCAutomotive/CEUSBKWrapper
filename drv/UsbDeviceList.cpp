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

#include "StdAfx.h"
#include "UsbDeviceList.h"
#include "UsbDevice.h"
#include "drvdbg.h"
#include "MutexLocker.h"
#include "ArrayAutoPtr.h"

#include <new>
#include <memory>

UsbDeviceList* UsbDeviceList::mSingleton;

UsbDeviceList* UsbDeviceList::Get()
{
	return mSingleton;
}

UsbDeviceList* UsbDeviceList::Create()
{
	if (!mSingleton) {
		mSingleton = new (std::nothrow) UsbDeviceList();
		if (!mSingleton->Init()) {
			delete mSingleton;
			mSingleton = NULL;
		}
	}
	return mSingleton;
}

void UsbDeviceList::DestroySingleton()
{
	delete mSingleton;
	mSingleton = NULL;
}

LONG UsbDeviceList::FindNextInterfaceFilterField(LPCWSTR str, DWORD offset, LPDWORD nextOffset, LPINTERFACE_FILTER_FIELD field)
{
	// Initialise field to match-all (*)
	field->match = TRUE;
	field->value = USB_NO_INFO;

	(*nextOffset) = offset;

	for (DWORD i = offset; i <= wcslen(str); i++) {
		// Field is terminated by ':', ';' or end of string
		if (str[i] == ':' || str[i] == ';' || str[i] == '\0') {
			// Represent ';' as reaching end of string (only flags occur after 
			// this seperator)
			if (str[i] == ';') {
				(*nextOffset) = wcslen(str) + 1;
			} else {
				(*nextOffset) = i + 1;
			}
			LONG result = ERROR_SUCCESS;

			// Empty fields are disallowed
			// Fields may contain '*' or a base 10 value (possibly prepended with '!'
			// to invert match).
			if (i == offset) {
				// Invalid (empty field)
				result = ERROR_INVALID_PARAMETER;
			} else if (!(i == offset + 1 && str[offset] == '*')) {
				// Field contains base 10 value
				wchar_t* endptr = NULL;
				if (str[offset] == '!') {
					field->match = FALSE;
					offset++;
				}
				field->value = wcstol(&str[offset], &endptr, 10);
				if (endptr == &str[offset] || 
					(endptr[0] != ':' && endptr[0] != ';' && endptr[0] != '\0')) {
					result = ERROR_INVALID_PARAMETER;
				}
			}
			return result;
		}
	}

	// Offset beyond end of string (this may occur if we're looking
	// for optional fields that are not present).
	return ERROR_INVALID_PARAMETER;
}

void UsbDeviceList::LogFilterField(LPCWSTR name, LPCINTERFACE_FILTER_FIELD field)
{
	if (field->value == USB_NO_INFO) {
		IFACEFILTER_MSG((TEXT("USBKWrapperDrv:     %s == <any>\r\n"), name));
	} else if (field->match) {
		IFACEFILTER_MSG((TEXT("USBKWrapperDrv:     %s == 0x%x\r\n"), name, field->value));
	} else {
		IFACEFILTER_MSG((TEXT("USBKWrapperDrv:     %s != 0x%x\r\n"), name, field->value));
	}
}

void UsbDeviceList::LogFilterFlag(LPCWSTR name, BOOL flag)
{
	// Only log the flags that are set explicitly.
	if (flag) {
		IFACEFILTER_MSG((TEXT("USBKWrapperDrv:     %s\r\n"), name));
	}
}

void UsbDeviceList::LogFilter(LPCWSTR message, LPCINTERFACE_FILTER filter)
{
	IFACEFILTER_MSG((TEXT("%s %s:\r\n"), message, filter->name));
	LogFilterField(TEXT("bInterfaceClass"), &filter->bInterfaceClass);
	LogFilterField(TEXT("bInterfaceSubClass"), &filter->bInterfaceSubClass);
	LogFilterField(TEXT("bInterfaceProtocol"), &filter->bInterfaceProtocol);
	LogFilterField(TEXT("idVendor"), &filter->idVendor);
	LogFilterField(TEXT("idProduct"), &filter->idProduct);

	LogFilterFlag(TEXT("NO_ATTACH"), filter->noAttach);
}

void UsbDeviceList::AddInterfaceFilter(LPCWSTR name, LPCWSTR value)
{
	// Filter name must be unique
	for (DWORD i = 0; i < mNumInterfaceFilters; i++) {
		if (wcscmp(name, mInterfaceFilters[i].name) == 0) {
			IFACEFILTER_MSG((TEXT("USBKWrapperDrv: Ignoring duplicate interface filter %s\r\n"), name));
			return;
		}
	}

	// Parse filter string "bInterfaceClass:bInterfaceSubclass:bInterfaceProtocol"
	// Any field can be '*', represent this "dont-care" as USB_NO_INFO.
	INTERFACE_FILTER_FIELD bInterfaceClass, bInterfaceSubClass, bInterfaceProtocol;
	DWORD nextOffset, result;

	result = FindNextInterfaceFilterField(value, 0, &nextOffset, &bInterfaceClass);
	result |= FindNextInterfaceFilterField(value, nextOffset, &nextOffset, &bInterfaceSubClass);
	result |= FindNextInterfaceFilterField(value, nextOffset, &nextOffset, &bInterfaceProtocol);
	if (result) {
		IFACEFILTER_MSG((TEXT("USBKWrapperDrv: Parse error for interface filter %s: %s\r\n"), name, value));
		return;		
	}

	// Now check for optional idVendor/idProduct fields (initialised by FindNextInterfaceFilterToken())
	INTERFACE_FILTER_FIELD idVendor, idProduct;
	FindNextInterfaceFilterField(value, nextOffset, &nextOffset, &idVendor);
	FindNextInterfaceFilterField(value, nextOffset, &nextOffset, &idProduct);

	PWCHAR filterName = (PWCHAR) malloc(sizeof(WCHAR)*(wcslen(name)+1));;
	if (!filterName) {
		ERROR_MSG((TEXT("USBKWrapperDrv!UsbDeviceList::AddInterfaceFilter() - Out of memory allocating filter name\r\n")));
		return;
	}
	wcscpy(filterName, name);

	// Finally, check for any optional flags (we can switch to a more advanced
	// flag parsing strategy if and when we add more flags in the future).
	BOOL noAttach = FALSE;
	if (wcsstr(value, L";NO_ATTACH") != NULL) {
		noAttach = TRUE;
	}

	INTERFACE_FILTER* newFilterList = (INTERFACE_FILTER*) realloc(mInterfaceFilters, sizeof(INTERFACE_FILTER)*(mNumInterfaceFilters+1));
	if (!newFilterList) {
		ERROR_MSG((TEXT("USBKWrapperDrv!UsbDeviceList::AddInterfaceFilter() - Out of memory reallocating filter list\r\n")));
		free(filterName);
		return;
	}

	// Append filter to list
	mInterfaceFilters = newFilterList;
	mInterfaceFilters[mNumInterfaceFilters].name = filterName;
	mInterfaceFilters[mNumInterfaceFilters].bInterfaceClass = bInterfaceClass;
	mInterfaceFilters[mNumInterfaceFilters].bInterfaceSubClass = bInterfaceSubClass;
	mInterfaceFilters[mNumInterfaceFilters].bInterfaceProtocol = bInterfaceProtocol;
	mInterfaceFilters[mNumInterfaceFilters].idVendor = idVendor;
	mInterfaceFilters[mNumInterfaceFilters].idProduct = idProduct;
	mInterfaceFilters[mNumInterfaceFilters].noAttach = noAttach;
	LogFilter(TEXT("USBKWrapperDrv: Added interface filter"), &mInterfaceFilters[mNumInterfaceFilters]);		
	mNumInterfaceFilters++;
}

void UsbDeviceList::FetchInterfaceFilters(LPCUSB_FUNCS lpUsbFuncs,
																					LPCWSTR szUniqueDriverId)
{
	HKEY key;
	LONG result;
	DWORD numValues = 0, maxNameLen = 0, maxValueLen = 0;
	DWORD valueType;
	PWCHAR nameBuf;
	PBYTE valueBuf;

	// We do not support dynamic updating of the filter list
	if (mNumInterfaceFilters > 0) {
		return;
	}
	
	key = lpUsbFuncs->lpOpenClientRegistyKey(szUniqueDriverId);
	if (!key) {
		ERROR_MSG((TEXT("USBKWrapperDrv!UsbDeviceList::FetchInterfaceFilters() - Failed to open client registry key\r\n")));
		return;
	}

	result = RegQueryInfoKey(key, NULL, NULL, NULL, NULL, NULL, NULL, 
		&numValues, &maxNameLen, &maxValueLen, NULL, NULL);
	if (result != ERROR_SUCCESS) {
		ERROR_MSG((TEXT("USBKWrapperDrv!UsbDeviceList::FetchInterfaceFilters() - Failed to query key info: error %i\r\n"), result));
		RegCloseKey(key);
		return;
	}

	nameBuf = new (std::nothrow) WCHAR[maxNameLen+1];
	valueBuf = new (std::nothrow) BYTE[maxValueLen];

	if (!nameBuf || !valueBuf) {
		ERROR_MSG((TEXT("USBKWrapperDrv!UsbDeviceList::FetchInterfaceFilters() - Out of memory allocating buffers\r\n")));		
		delete[] nameBuf;
		delete[] valueBuf;
		RegCloseKey(key);
		return;
	}

	for (DWORD index = 0; index < numValues && result == ERROR_SUCCESS; index++) {
		DWORD nameBufSize = maxNameLen + 1;
		DWORD valueBufSize = maxValueLen;
		result = RegEnumValue(key, index, nameBuf, &nameBufSize, NULL, &valueType, 
			valueBuf, &valueBufSize);

		if (result == ERROR_SUCCESS) {
			// Pay attention only to unicode strings (REG_SZ) whose name has the 
			// InterfaceFilter_ prefix
			if (nameBufSize > 16 && wcsncmp(nameBuf, TEXT("InterfaceFilter_"), 16) == 0 && 
				valueType == REG_SZ) {
				AddInterfaceFilter(&nameBuf[16], (LPCWSTR) valueBuf);
			}
		}
	}

	delete[] nameBuf;
	delete[] valueBuf;
	RegCloseKey(key);
}

void UsbDeviceList::DestroyInterfaceFilters()
{
	for (DWORD i = 0; i < mNumInterfaceFilters; i++) {
		free(mInterfaceFilters[i].name);
	}
	free(mInterfaceFilters);
	mInterfaceFilters = NULL;
	mNumInterfaceFilters = 0;
}

BOOL UsbDeviceList::MatchFilterField(BOOL doComparison, DWORD value, LPCINTERFACE_FILTER_FIELD field)
{
	if (!doComparison) {
		return FALSE;
	} else if (field->value == USB_NO_INFO) {
		return TRUE;
	} else if (field->match) {
		return (field->value == value);
	} else {
		return (field->value != value);
	}
}

BOOL UsbDeviceList::FindFilterForInterface(LPCUSB_DEVICE lpDevice, LPCUSB_INTERFACE lpInterface, LPDWORD index)
{
	for (DWORD i = 0; i < mNumInterfaceFilters; i++) {
		BOOL match = MatchFilterField(true, lpInterface->Descriptor.bInterfaceClass, &mInterfaceFilters[i].bInterfaceClass);
		match &= MatchFilterField(match, lpInterface->Descriptor.bInterfaceSubClass, &mInterfaceFilters[i].bInterfaceSubClass);
		match &= MatchFilterField(match, lpInterface->Descriptor.bInterfaceProtocol, &mInterfaceFilters[i].bInterfaceProtocol);
		match &= MatchFilterField(match, lpDevice->Descriptor.idVendor, &mInterfaceFilters[i].idVendor);
		match &= MatchFilterField(match, lpDevice->Descriptor.idProduct, &mInterfaceFilters[i].idProduct);

		if (match) {
			(*index) = i;
			return TRUE;
		}
	}

	// No match
	return FALSE;
}

BOOL UsbDeviceList::AttachDevice(
	USB_HANDLE hDevice,
	LPCUSB_FUNCS lpUsbFuncs,
	LPCUSB_INTERFACE lpInterface,
	LPCWSTR szUniqueDriverId,
	LPBOOL fAcceptControl,
	LPCUSB_DRIVER_SETTINGS lpDriverSettings,
	DWORD dwUnused)
{
	unsigned char bus = 0, address = 0;
	unsigned long session_id = 0;

	MutexLocker lock(mMutex);
	(void) szUniqueDriverId;
	// In future allowing some filtering based on lpDriverSettings
	// here could be useful.
	(void) lpDriverSettings;
	(void) dwUnused;

	// Default to accepting control of device, until we find a reason not to.
	*fAcceptControl = TRUE;

	LPCUSB_DEVICE device = lpUsbFuncs->lpGetDeviceInfo(hDevice);

	// Retrieve interface filter list from registry if not already done so
	FetchInterfaceFilters(lpUsbFuncs, szUniqueDriverId);

	// If we're attaching for an interface only, then refuse if the interface is
	// in our filter list. Else, if we're attaching for a device only then refuse if
	// we are already attached (we may be trying to find other drivers).
	if (lpInterface) {
		DWORD index;
		if (FindFilterForInterface(device, lpInterface, &index)) {
			IFACEFILTER_MSG((TEXT("USBKWrapperDrv: Interface %i matches filter %s, not attaching\r\n"), 
				lpInterface->Descriptor.bInterfaceNumber, mInterfaceFilters[index].name));
			(*fAcceptControl) = FALSE;
			return FALSE;
		}
	} else {
		for (PtrArray<UsbDevice>::iterator it = mDevices.begin(); it != mDevices.end(); ++it) {
			if ((*it)->IsSameDevice(hDevice)) {
				WARN_MSG((TEXT("USBKWrapperDrv: Already attached to device, not attaching again\r\n")));
				(*fAcceptControl) = FALSE;
				return FALSE;
			}
		}
	}

	// If we're attaching for the entire device, locate any interface filters 
	// matching the interfaces of this device.
	//
	// If any matching filter specifies NO_ATTACH, we refuse to accept control
	// of the device. We only match one filter to an interface. If multiple 
	// filters match, only the first shall be used.
	BOOL filterMatches = FALSE;
	ArrayAutoPtr<DWORD> filters;
	if (!lpInterface && mNumInterfaceFilters > 0) {
		filters = new (std::nothrow) DWORD[device->lpActiveConfig->dwNumInterfaces];
		if (!filters.Get()) {
			ERROR_MSG((TEXT("USBKWrapperDrv!UsbDeviceList::AttachDevice")
				TEXT(" - out of memory allocating filter match list")));
			*fAcceptControl = FALSE;
			return FALSE;
		}

		for (DWORD i = 0; i < device->lpActiveConfig->dwNumInterfaces && *fAcceptControl; i++) {
			DWORD index;
			if (FindFilterForInterface(device, &device->lpActiveConfig->lpInterfaces[i], &index)) {
				filters.Set(i, index);
				filterMatches = TRUE;

				if (mInterfaceFilters[index].noAttach) {
					IFACEFILTER_MSG((TEXT("USBKWrapperDrv: Interface %u matches filter %s")
						TEXT(", not accepting device control\r\n"), 
						device->lpActiveConfig->lpInterfaces[i].Descriptor.bInterfaceNumber, 
						mInterfaceFilters[index].name));
					*fAcceptControl = FALSE;
				}
			} else {
				filters.Set(i, mNumInterfaceFilters);
			}
		}
	}

	// If we're not accepting control, finish now
	if (!*fAcceptControl) {
		return FALSE;
	}

	// Allocate a fake bus and address for this device
	// as WinCE doesn't expose the bus and address to
	// USB Function Drivers.
	if (!mBusAllocator.Alloc(bus, address, session_id)) {
		ERROR_MSG((TEXT("USBKWrapperDrv!UsbDeviceList::AttachDevice")
			TEXT(" - failed to allocate fake USB bus and adress for handle 0x%08x\r\n"), hDevice));
		*fAcceptControl = FALSE;
		return FALSE;
	}
	std::auto_ptr<UsbDevice> ptr (
		new (std::nothrow) UsbDevice(bus, address, session_id));
	if (!ptr.get()) {
		mBusAllocator.Free(bus, address, session_id);
		ERROR_MSG((TEXT("USBKWrapperDrv!UsbDeviceList::AttachDevice")
			TEXT(" - failed to allocate new USB device for handle 0x%08x\r\n"), hDevice));
		*fAcceptControl = FALSE;
		return FALSE;
	}
	if (!ptr.get()->Init(hDevice, lpUsbFuncs, lpInterface)) {
		ERROR_MSG((TEXT("USBKWrapperDrv!UsbDeviceList::AttachDevice")
			TEXT(" - failed to initialise new USB device for handle 0x%08x\r\n"), hDevice));
		*fAcceptControl = FALSE;
		return FALSE;
	}
	if (!mDevices.insert(ptr.get())) {
		ERROR_MSG((TEXT("USBKWrapperDrv!UsbDeviceList::AttachDevice")
			TEXT(" - failed to store new USB device for handle 0x%08x\r\n"), hDevice));
		*fAcceptControl = FALSE;
		return FALSE;
	}

	// Release any interfaces matching filters
	if (filterMatches) {
		// Try to release any interfaces that match our filters (if we have any)
		BOOL failedToAttach = FALSE;
		BOOL attachSuccesful = FALSE;
		for (DWORD i = 0; i < device->lpActiveConfig->dwNumInterfaces && !failedToAttach; i++) {
			if (filters.Get(i) < mNumInterfaceFilters) {
				UCHAR bInterfaceNumber = device->lpActiveConfig->lpInterfaces[i].Descriptor.bInterfaceNumber;
				IFACEFILTER_MSG((TEXT("USBKWrapperDrv: Interface %u matches filter %s")
					TEXT(", attaching kernel driver\r\n"), bInterfaceNumber, mInterfaceFilters[filters.Get(i)].name));
				if (ptr.get()->AttachKernelDriverForInterface(bInterfaceNumber)) {
					attachSuccesful = TRUE;
				} else if (!attachSuccesful) {
					// Only resort to finding a driver for the entire device, if we have
					// not already attached drivers to other interfaces.
					failedToAttach = TRUE;
				}
			}
		}
		if (failedToAttach) {
			// Try to find a driver for the entire device. We will release all of our interfaces,
			// but still reserve the right to send arbitrary control transfers.
			IFACEFILTER_MSG((TEXT("USBKWrapperDrv: Attaching kernel driver for entire device\r\n")));
			if (!ptr.get()->AttachKernelDriverForDevice()) {
				// There really is no other driver willing to control this device!
				IFACEFILTER_MSG((TEXT("USBKWrapperDrv: Unable to attach kernel driver for entire ")
					TEXT("device (error %u), retaining control\r\n"), GetLastError()));
			}
		}
	}

	ptr.release();
	return TRUE;
}

UsbDevice* UsbDeviceList::GetDevice(UKWD_USB_DEVICE identifier)
{
	MutexLocker lock(mMutex);
	PtrArray<UsbDevice>::iterator it
		= mDevices.find(static_cast<UsbDevice*>(identifier));
	if (it == mDevices.end()) {
	    WARN_MSG((TEXT("USBKWrapperDrv!UsbDeviceList::GetDevice")
			TEXT(" - failed to find device for identifier 0x%08x\r\n"), identifier));
		return NULL;
	} else {
		(*it)->IncRef();
	}
	return *it;
}

UsbDevice* UsbDeviceList::GetDevice(UsbDevice* device)
{
	MutexLocker lock(mMutex);
	device->IncRef();
	return device;
}

void UsbDeviceList::PutDevice(UsbDevice* device)
{
	MutexLocker lock(mMutex);
	if (device) {
		DWORD count = device->DecRef();
		if (count <= 0) {
			mBusAllocator.Free(
				device->Bus(),
				device->Address(),
				device->SessionId());
			mDevices.erase(device);
			delete device;
		}
	}
}


DWORD UsbDeviceList::GetAvailableDevices(UsbDevice** lpDevices, DWORD Size)
{
	MutexLocker lock(mMutex);
	DWORD count = 0;
	PtrArray<UsbDevice>::iterator it = mDevices.begin();
	while(it != mDevices.end() && Size > 0) {
		if (!(*it)->Closed()) {
			lpDevices[count] = *it;
			(*it)->IncRef();
			--Size;
			++count;
		}
		++it;
	}
	return count;
}

UsbDeviceList::UsbDeviceList()
: mMutex(NULL),
mInterfaceFilters(NULL),
mNumInterfaceFilters(0)
{
}

BOOL UsbDeviceList::Init()
{
	mMutex = CreateMutex(NULL, FALSE, NULL);
	if (mMutex == NULL) {
		ERROR_MSG((TEXT("USBKWrapperDrv!UsbDeviceList::Init() - failed to create mutex\r\n")));
		return FALSE;
	}
	return TRUE;
}


UsbDeviceList::~UsbDeviceList()
{
	if (mMutex) {
		CloseHandle(mMutex);
		mMutex = NULL;
	}

	DestroyInterfaceFilters();

	PtrArray<UsbDevice>::const_iterator iter =
		mDevices.begin();
	while (iter != mDevices.end()) {
		delete *iter;
		++iter;
	}
}

