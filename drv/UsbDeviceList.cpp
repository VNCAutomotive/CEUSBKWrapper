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

BOOL UsbDeviceList::ParseFilterString(INTERFACE_FILTER& filter, LPCWSTR str) 
{
	// Parse a filter string, which has the following syntax:
	//
	// [priority;]bInterfaceClass:bInterfaceSubClass:bInterfaceProtocol[:idVendor[:idProduct]][;NO_ATTACH]
	//	
	// Apart from priority and flag fields, any field can be '*', represent this "dont-care" 
	// as USB_NO_INFO.
	// The filter string may optionally start with a priority value, seperated by a
	// semi-colon. MAXDWORD represents no priority, and is the default.
	// The filter string may optionally terminate with a series of flags, again 
	// seperated by a semi-colon. Currently, the only flag supported is NO_ATTACH.
	//
	// The INTERFACE_FILTER must already have a valid name.

	// The parsing states
	typedef enum {
		ParseStateStart,
		ParseStateInterfaceClass,
		ParseStateInterfaceSubClass,
		ParseStateInterfaceProtocol,
		ParseStateIdVendor,
		ParseStateIdProduct,
		ParseStateFlags,

		ParseStateEnd

	} ParseState;

	// Set filter defaults
	filter.priority = MAXDWORD;
	filter.noAttach = FALSE;
	FillInFilterField(filter.bInterfaceClass, 0, TRUE, FALSE);
	FillInFilterField(filter.bInterfaceSubClass, 0, TRUE, FALSE);
	FillInFilterField(filter.bInterfaceProtocol, 0, TRUE, FALSE);
	FillInFilterField(filter.idVendor, 0, TRUE, FALSE);
	FillInFilterField(filter.idProduct, 0, TRUE, FALSE);

	ParseState parseState = ParseStateStart;
	DWORD offset = 0;
	BOOL result = TRUE;
	for (DWORD i = 0; i <= wcslen(str) && result; i++) {
		// Each field is terminated by ':', ';' or end of string
		// No field may be empty
		if (str[i] == ':' || str[i] == ';' || str[i] == '\0') {
			DWORD parseValue;
			BOOL parseMatchAll = FALSE;
			BOOL parseInvert = FALSE;

			if (i == offset || (i == offset + 1 && str[offset] == '!')) {
				// Empty or malformed field
				ERROR_MSG((TEXT("USBKWrapperDrv: Empty or malformed field in interface filter %s:%u\r\n"), 
					filter.name, offset));
				result = FALSE;
				break;
			} else if (parseState < ParseStateFlags) {
				// Parse the numeric field generically.
				if (i == offset + 1 && str[offset] == '*') {
					parseMatchAll = TRUE;
				} else {
					wchar_t* endptr = NULL;
					if (str[offset] == '!') {
						parseInvert = TRUE;
						++offset;
					}
					parseValue = wcstol(&str[offset], &endptr, 10);
					if (endptr != &str[i]) {
						ERROR_MSG((TEXT("USBKWrapperDrv: Invalid field value in interface filter %s:%u\r\n"), 
							filter.name, offset));
						result = FALSE;
						break;
					}
				}
			}

			// Now interpret the field depending on what stage we're at
			switch (parseState) {
				/** Numeric fields **/

				case ParseStateStart:
					// If token is ';', then parse optional priority field.
					// '*' and '!' operators are not valid for this field.
					// Otherwise, drop through and parse interface class field.
					if (str[i] == ';') {
						if (!parseMatchAll && !parseInvert) {
							filter.priority = parseValue;
							parseState = ParseStateInterfaceClass;
						} else {
							ERROR_MSG((TEXT("USBKWrapperDrv: Invalid priority value in interface filter %s:%u\r\n"), 
								filter.name, offset));
							result = FALSE;
						}
						break;
					}
					// Deliberate fall-through if token is not ';'

				case ParseStateInterfaceClass:
					FillInFilterField(filter.bInterfaceClass, 
								parseValue, 
								parseMatchAll, 
								parseInvert);

					// Check for required token
					if (str[i] == ':') {
						parseState = ParseStateInterfaceSubClass;
					} else {
						ERROR_MSG((TEXT("USBKWrapperDrv: Expected ':' in interface filter %s:%u\r\n"),
							filter.name, i));
						result = FALSE;
					}
					break;

				case ParseStateInterfaceSubClass:
					FillInFilterField(filter.bInterfaceSubClass, 
								parseValue, 
								parseMatchAll, 
								parseInvert);

					// Check for required token
					if (str[i] == ':') {
						parseState = ParseStateInterfaceProtocol;
					} else {
						ERROR_MSG((TEXT("USBKWrapperDrv: Expected ':' in interface filter %s:%u\r\n"),
							filter.name, i));
						result = FALSE;
					}
					break;

				case ParseStateInterfaceProtocol:
					FillInFilterField(filter.bInterfaceProtocol, 
								parseValue, 
								parseMatchAll, 
								parseInvert);

					// Transition state based on token
					if (str[i] == ':') {
						parseState = ParseStateIdVendor;
					} else if (str[i] == ';') {
						parseState = ParseStateFlags;
					} else {
						parseState = ParseStateEnd;
					}
					break;

				case ParseStateIdVendor:
					FillInFilterField(filter.idVendor, 
								parseValue,
								parseMatchAll, 
								parseInvert);

					// Transition state based on token
					if (str[i] == ':') {
						parseState = ParseStateIdProduct;
					} else if (str[i] == ';') {
						parseState = ParseStateFlags;
					} else if (str[i] == '\0') {
						parseState = ParseStateEnd;
					}
					break;

				case ParseStateIdProduct:
					FillInFilterField(filter.idProduct, 
								parseValue, 
								parseMatchAll, 
								parseInvert);

					// Transition state based on token
					if (str[i] == ';') {
						parseState = ParseStateFlags;
					} else if (str[i] == '\0') {
						parseState = ParseStateEnd;
					} else {
						ERROR_MSG((TEXT("USBKWrapperDrv: Expected ';' or end of string in interface filter %s:%u\r\n"),
							filter.name, i));
						result = FALSE;
					}
					break;

				/** Optional (and non-numeric) flags **/

				case ParseStateFlags:
					// Parse the next flag
					// At the moment, we only support NO_ATTACH
					if (wcslen(&str[offset]) >= 9 && wcsncmp(&str[offset], L"NO_ATTACH", 9) == 0) {
						filter.noAttach = TRUE;
					} else {
						// Unrecognised flag
						ERROR_MSG((TEXT("USBKWrapperDrv: Unrecognised flag in interface filter %s:%u\r\n"),
							filter.name, offset));
						result = FALSE;
					}

					// Transition state based on token
					if (result) {
						if (str[i] == '\0') {
							parseState = ParseStateEnd;
						} else if (str[i] != ';') {
							ERROR_MSG((TEXT("USBKWrapperDrv: Expected ';' or end of string in interface filter %s:%u\r\n"),
								filter.name, i));
							result = FALSE;
						}
					}
					break;
			}

			// Update offset for start of next field
			offset = i + 1;
		}
	}
	return result;
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
	if (filter->priority == MAXDWORD) {
		IFACEFILTER_MSG((TEXT("USBKWrapperDrv:     priority: <none>\r\n")));
	} else {
		IFACEFILTER_MSG((TEXT("USBKWrapperDrv:     priority:  %u\r\n"), filter->priority));
	}

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
	PFILTER_NODE next = mInterfaceFilters;
	while (next) {
		if (wcscmp(name, next->filter.name) == 0) {
			IFACEFILTER_MSG((TEXT("USBKWrapperDrv: Ignoring duplicate interface filter %s\r\n"), name));
			return;
		}
		next = next->next;
	}

	// Prepare new filter
	PFILTER_NODE newFilter = new (std::nothrow) FILTER_NODE;
	if (!newFilter) {
		ERROR_MSG((TEXT("USBKWrapperDrv!UsbDeviceList::AddInterfaceFilter() - Out of memory allocating new filter node\r\n")));
		return;
	}
	newFilter->filter.name = new (std::nothrow) WCHAR[wcslen(name) + 1];
	if (!newFilter->filter.name) {
		ERROR_MSG((TEXT("USBKWrapperDrv!UsbDeviceList::AddInterfaceFilter() - Out of memory allocating filter name\r\n")));
		delete newFilter;
		return;
	}
	wcscpy(newFilter->filter.name, name);

	// Parse filter string
	if (!ParseFilterString(newFilter->filter, value)) {
		IFACEFILTER_MSG((TEXT("USBKWrapperDrv: Discarding invalid interface filter %s: %s\r\n"), name, value));
		// Destroy filter
		delete[] newFilter->filter.name;
		delete newFilter;
		return;		
	}

	// Insert filter into the correct position in the list (dependent on priority)
	if (!mInterfaceFilters || newFilter->filter.priority <= mInterfaceFilters->filter.priority) {
		// Insert at the head
		newFilter->next = mInterfaceFilters;
		mInterfaceFilters = newFilter;
	} else {
		next = mInterfaceFilters;
		while (next) {
			if (!next->next || newFilter->filter.priority <= next->next->filter.priority) {
				// Insert after this element (which may be the tail)
				newFilter->next = next->next;
				next->next = newFilter;
				break;
			}
			next = next->next;
		}
	}
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
	if (mInterfaceFilters) {
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

	// Log out the interface filters we have, in priority order
	PFILTER_NODE next = mInterfaceFilters;
	while(next) {
		LogFilter(TEXT("USBKWrapperDrv: Added interface filter"), &next->filter);
		next = next->next;
	}

	delete[] nameBuf;
	delete[] valueBuf;
	RegCloseKey(key);
}

void UsbDeviceList::DestroyInterfaceFilters()
{
	PFILTER_NODE next = mInterfaceFilters;
	while (next) {
		PFILTER_NODE del = next;
		next = del->next;
		delete[] del->filter.name;
		delete del;
	}
	mInterfaceFilters = NULL;
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

BOOL UsbDeviceList::FindFilterForInterface(LPCUSB_DEVICE lpDevice, LPCUSB_INTERFACE lpInterface, LPINTERFACE_FILTER* filter)
{
	// Filter list is maintained in priority order
	PFILTER_NODE next = mInterfaceFilters;
	while (next) {
		BOOL match = MatchFilterField(true, lpInterface->Descriptor.bInterfaceClass, &next->filter.bInterfaceClass);
		match &= MatchFilterField(match, lpInterface->Descriptor.bInterfaceSubClass, &next->filter.bInterfaceSubClass);
		match &= MatchFilterField(match, lpInterface->Descriptor.bInterfaceProtocol, &next->filter.bInterfaceProtocol);
		match &= MatchFilterField(match, lpDevice->Descriptor.idVendor, &next->filter.idVendor);
		match &= MatchFilterField(match, lpDevice->Descriptor.idProduct, &next->filter.idProduct);

		if (match) {
			(*filter) = &next->filter;
			return TRUE;
		}

		next = next->next;
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
		PINTERFACE_FILTER filter;
		if (FindFilterForInterface(device, lpInterface, &filter)) {
			IFACEFILTER_MSG((TEXT("USBKWrapperDrv: Interface %i matches filter %s, not attaching\r\n"), 
				lpInterface->Descriptor.bInterfaceNumber, filter->name));
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
	ArrayAutoPtr<PINTERFACE_FILTER> filters;
	if (!lpInterface && mInterfaceFilters) {
		filters = new (std::nothrow) PINTERFACE_FILTER[device->lpActiveConfig->dwNumInterfaces];
		if (!filters.Get()) {
			ERROR_MSG((TEXT("USBKWrapperDrv!UsbDeviceList::AttachDevice")
				TEXT(" - out of memory allocating filter match list")));
			*fAcceptControl = FALSE;
			return FALSE;
		}

		for (DWORD i = 0; i < device->lpActiveConfig->dwNumInterfaces && *fAcceptControl; i++) {
			PINTERFACE_FILTER filter;
			if (FindFilterForInterface(device, &device->lpActiveConfig->lpInterfaces[i], &filter)) {
				filters.Set(i, filter);
				filterMatches = TRUE;

				if (filter->noAttach) {
					IFACEFILTER_MSG((TEXT("USBKWrapperDrv: Interface %u matches filter %s")
						TEXT(", not accepting device control\r\n"), 
						device->lpActiveConfig->lpInterfaces[i].Descriptor.bInterfaceNumber, 
						filter->name));
					*fAcceptControl = FALSE;
				}
			} else {
				filters.Set(i, NULL);
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
			if (filters.Get(i)) {
				UCHAR bInterfaceNumber = device->lpActiveConfig->lpInterfaces[i].Descriptor.bInterfaceNumber;
				IFACEFILTER_MSG((TEXT("USBKWrapperDrv: Interface %u matches filter %s")
					TEXT(", attaching kernel driver\r\n"), bInterfaceNumber, filters.Get(i)->name));
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
mInterfaceFilters(NULL)
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

