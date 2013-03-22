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

// UserBuffer.cpp : Represents a user buffer

#include "StdAfx.h"
#include "UserBuffer.h"
#include "drvdbg.h"

#include <pkfuncs.h>
#include <ceddk.h>
#include <memory>

#if _WIN32_WCE >= 0x600
/* WinCE 6 and beyond support */

template<typename T>
UserBuffer<T>::UserBuffer(
		DWORD dwAccessFlags,
		T lpSrcUnmarshalled,
		DWORD dwSize)
: mlpSrcUnmarshalled(lpSrcUnmarshalled)
, mSize(dwSize)
, mAsync((dwAccessFlags & UBA_ASYNC) == UBA_ASYNC)
, mArgDesc(ArgDescFromAccessFlags(dwAccessFlags))
, mlpSyncMarshalled(NULL)
, mlpAsyncMarshalled(NULL)
{
	if (!mlpSrcUnmarshalled)
		return;
	if (mSize == 0)
		return;
	HRESULT r = CeOpenCallerBuffer(
		&mlpSyncMarshalled, mlpSrcUnmarshalled,
		mSize, mArgDesc,
		((dwAccessFlags & UBA_FORCE_DUPLICATE) == UBA_FORCE_DUPLICATE));
	if (FAILED(r)) {
		ERROR_MSG((TEXT("USBKWrapperDrv!UserBuffer::UserBuffer() failed to open caller buffer 0x%08x with error 0x%08x\r\n"),
			lpSrcUnmarshalled, r));
		mlpSyncMarshalled = NULL;
	}
	if (mlpSyncMarshalled != NULL && mAsync) {
		HRESULT r = CeAllocAsynchronousBuffer(&mlpAsyncMarshalled, mlpSyncMarshalled, mSize, mArgDesc);
		if (FAILED(r)) {
			ERROR_MSG((TEXT("USBKWrapperDrv!UserBuffer::UserBuffer() failed to alloc async buffer 0x%08x with error 0x%08x\r\n"),
				lpSrcUnmarshalled, r));
			mlpAsyncMarshalled = NULL;
		}
	}
}

template<typename T>
UserBuffer<T>::~UserBuffer()
{
	if (mlpAsyncMarshalled != NULL) {
		HRESULT r = CeFreeAsynchronousBuffer(mlpAsyncMarshalled, mlpSyncMarshalled, mSize, mArgDesc);
		if (FAILED(r)) {
			ERROR_MSG((TEXT("USBKWrapperDrv!UserBuffer::~UserBuffer() failed to free async buffer %d\r\n"),
				r));
		}
		mlpAsyncMarshalled = NULL;
	}
	if (mlpSyncMarshalled != NULL) {
		HRESULT r = CeCloseCallerBuffer(mlpSyncMarshalled, mlpSrcUnmarshalled, mSize, mArgDesc);
		if (FAILED(r)) {
			ERROR_MSG((TEXT("USBKWrapperDrv!UserBuffer::~UserBuffer() failed to close caller buffer %d\r\n"),
				r));
		}
		mlpSyncMarshalled = NULL;
	}
}

template<typename T>
BOOL UserBuffer<T>::Flush()
{
	if (mSize == 0) {
		return TRUE;
	}
	if (mlpAsyncMarshalled == NULL) {
		return !mAsync;
	}
	HRESULT r = CeFlushAsynchronousBuffer(
		mlpAsyncMarshalled, mlpSyncMarshalled, mlpSrcUnmarshalled,
		mSize, mArgDesc);
	if (FAILED(r)) {
		ERROR_MSG((TEXT("USBKWrapperDrv!UserBuffer::Flush failed to flush async buffer %d\r\n"),
			r));
	}
	return SUCCEEDED(r);
}

template<typename T>
DWORD UserBuffer<T>::ArgDescFromAccessFlags(DWORD dwAccessFlags)
{
	DWORD ret;
	DWORD rw = dwAccessFlags & UBA_READ_WRITE;
	switch(rw) {
	default:
	case UBA_READ_WRITE:
		ret = ARG_IO_PTR;
		break;
	case UBA_READ:
		ret = ARG_I_PTR;
		break;
	case UBA_WRITE:
		ret = ARG_O_PTR;
		break;
	}
	return ret;
}

template<>
DWORD UserBuffer<LPDWORD>::ArgDescFromAccessFlags(DWORD dwAccessFlags)
{
	DWORD ret;
	DWORD rw = dwAccessFlags & UBA_READ_WRITE;
	switch(rw) {
	default:
	case UBA_READ_WRITE:
		ret = ARG_IO_PDW;
		break;
	case UBA_READ:
		ret = ARG_I_PDW;
		break;
	case UBA_WRITE:
		ret = ARG_O_PDW;
		break;
	}
	return ret;
}

OverlappedUserBuffer::OverlappedUserBuffer(LPOVERLAPPED lpOverlapped)
: UserBuffer<LPOVERLAPPED>(UBA_WRITE | UBA_ASYNC, lpOverlapped, sizeof(OVERLAPPED))
, mhEvent(INVALID_HANDLE_VALUE)
, mCompleted(FALSE)
{
	if (!Valid() || Ptr() == NULL)
		return;
	// Need to duplicate the handle into kernel space
	mhEvent = CeDriverDuplicateCallerHandle(operator->().hEvent, 0, FALSE, DUPLICATE_SAME_ACCESS);
	if (mhEvent == NULL) {
		ERROR_MSG((TEXT("USBKWrapperDrv!OverlappedUserBuffer::OverlappedUserBuffer failed to duplicate handle\r\n")));
		mhEvent = INVALID_HANDLE_VALUE;
		return;
	}
	// Set the status as pending
	operator->().Internal = STATUS_PENDING;
	operator->().InternalHigh = 0;
	// Write the changes back to userspace
	Flush();
}

#else /* _WIN32_WCE >= 0x600 */
/* WinCE 5 and below support */

template<typename T>
UserBuffer<T>::UserBuffer(
		DWORD dwAccessFlags,
		T lpSrcUnmarshalled,
		DWORD dwSize)
: mlpSrcUnmarshalled(lpSrcUnmarshalled)
, mSize(dwSize)
, mAsync((dwAccessFlags & UBA_ASYNC) == UBA_ASYNC)
, mArgDesc(ArgDescFromAccessFlags(dwAccessFlags))
, mlpSyncMarshalled(NULL)
, mlpAsyncMarshalled(NULL)
{
	if (!mlpSrcUnmarshalled)
		return;
	if (mSize == 0)
		return;
	mlpSyncMarshalled = MapCallerPtr(lpSrcUnmarshalled, mSize);
	if (!mlpSyncMarshalled) {
		ERROR_MSG((TEXT("USBKWrapperDrv!UserBuffer::UserBuffer() failed to map caller buffer 0x%08x\r\n"),
			lpSrcUnmarshalled));
		mlpSyncMarshalled = NULL;
	}
	if (mlpSyncMarshalled != NULL && mAsync) {
		mlpAsyncMarshalled = malloc(mSize);
		if (!mlpAsyncMarshalled) {
			ERROR_MSG((TEXT("USBKWrapperDrv!UserBuffer::UserBuffer() failed to alloc async buffer 0x%08x\r\n"),
				lpSrcUnmarshalled));
			mlpAsyncMarshalled = NULL;
		} else {
			/* MapCallerPtr has checked that this is a valid user space region. */
			memcpy(mlpAsyncMarshalled, mlpSyncMarshalled, mSize);
		}
	}
}

template<typename T>
UserBuffer<T>::~UserBuffer()
{
	if (mlpAsyncMarshalled != NULL) {
		free(mlpAsyncMarshalled);
		mlpAsyncMarshalled = NULL;
	}
}


template<typename T>
BOOL UserBuffer<T>::Flush()
{
	if (mSize == 0) {
		return TRUE;
	}
	if (mlpAsyncMarshalled == NULL) {
		return !mAsync;
	}
	/* Switch back to the callers permissions and then memcpy the data back */
	const DWORD oldPermissions = SetProcPermissions(mArgDesc);
	memcpy(mlpSyncMarshalled, mlpAsyncMarshalled, mSize);
	DWORD argDesc = SetProcPermissions(oldPermissions);
	BOOL ret = TRUE;
	if (argDesc != mArgDesc) {
		ERROR_MSG((TEXT("USBKWrapperDrv!UserBuffer::Flush failed to flush async buffer %d != %d\r\n"),
			argDesc, mArgDesc));
		ret = FALSE;
	}
	return ret;
}

template<typename T>
DWORD UserBuffer<T>::ArgDescFromAccessFlags(DWORD dwAccessFlags)
{
	return GetCurrentPermissions();
}

OverlappedUserBuffer::OverlappedUserBuffer(LPOVERLAPPED lpOverlapped)
: UserBuffer<LPOVERLAPPED>(UBA_WRITE | UBA_ASYNC, lpOverlapped, sizeof(OVERLAPPED))
, mhEvent(INVALID_HANDLE_VALUE)
, mCompleted(FALSE)
{
	if (!Valid() || Ptr() == NULL)
		return;
	/* Need to duplicate the handle into kernel space */
	BOOL success = DuplicateHandle(
		GetOwnerProcess(), operator->().hEvent,
		GetCurrentProcess(), &mhEvent,
		0, FALSE, DUPLICATE_SAME_ACCESS);
	if (!success) {
		ERROR_MSG((TEXT("USBKWrapperDrv!OverlappedUserBuffer::OverlappedUserBuffer failed to duplicate handle\r\n")));
		mhEvent = INVALID_HANDLE_VALUE;
		return;
	}
	// Set the status as pending
	operator->().Internal = STATUS_PENDING;
	operator->().InternalHigh = 0;
	// Write the changes back to userspace
	Flush();
}


#endif /* _WIN32_WCE >= 0x600 */
/* Generic functions common to any WinCE version */

template<typename T>
BOOL UserBuffer<T>::Valid() const
{
	return mSize == 0 ||
		!mlpSrcUnmarshalled ||
		(mlpAsyncMarshalled && mAsync) ||
		(mlpSyncMarshalled && !mAsync);
}

template<typename T>
T UserBuffer<T>::UserPtr()
{
	return static_cast<T>(mlpSrcUnmarshalled);
}

template<typename T>
T UserBuffer<T>::Ptr()
{
	return static_cast<T>(
		mAsync ? 
		mlpAsyncMarshalled :
		mlpSyncMarshalled);
}

template<typename T>
DWORD UserBuffer<T>::Size()
{
	return mSize;
}

// Create versions of UserBuffer for supported types
template class UserBuffer<LPVOID>;
template class UserBuffer<LPDWORD>;
template class UserBuffer<LPOVERLAPPED>;

OverlappedUserBuffer::~OverlappedUserBuffer()
{
	if (!Completed() && Valid()) {
		// This should never happen if everything is operating correctly
		// but is needed to close the handle
		Complete(STATUS_DRIVER_INTERNAL_ERROR, 0);
	}
}

OVERLAPPED& OverlappedUserBuffer::operator->()
{
	return *Ptr();
}

BOOL OverlappedUserBuffer::Completed() const
{
	return mCompleted;
}

void OverlappedUserBuffer::Complete(DWORD dwStatus, DWORD dwBytesTransferred)
{
	if (Completed() || !Valid() || Ptr() == NULL)
		return;
	operator->().Internal = dwStatus;
	operator->().InternalHigh = dwBytesTransferred;
	// Need to flush the status before signalling the event
	Flush();
	SetEvent(mhEvent);
	// No need for the handle so close it
	CloseHandle(mhEvent);
	mhEvent = INVALID_HANDLE_VALUE;
	mCompleted = TRUE;
}

void OverlappedUserBuffer::Abort()
{
	if (Completed() || !Valid() || Ptr() == NULL)
		return;
	CloseHandle(mhEvent);
	mhEvent = INVALID_HANDLE_VALUE;
	mCompleted = TRUE;
}