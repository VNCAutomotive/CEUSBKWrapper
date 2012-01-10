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

// UserBuffer.cpp : Represents a user buffer

#include "StdAfx.h"
#include "UserBuffer.h"
#include "drvdbg.h"

#include <pkfuncs.h>
#include <ceddk.h>
#include <memory>

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
		ERROR_MSG((TEXT("USBKWrapperDrv!UserBuffer::UserBuffer() failed to open caller buffer 0x%08x with error %0x08x\r\n"),
			lpSrcUnmarshalled, r));
		mlpSyncMarshalled = NULL;
	}
	if (mlpSyncMarshalled != NULL && mAsync) {
		HRESULT r = CeAllocAsynchronousBuffer(&mlpAsyncMarshalled, mlpSyncMarshalled, mSize, mArgDesc);
		if (FAILED(r)) {
			ERROR_MSG((TEXT("USBKWrapperDrv!UserBuffer::UserBuffer() failed to alloc async buffer 0x%08x with error %0x08x\r\n"),
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
DWORD UserBuffer<T>::Size()
{
	return mSize;
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

// Create versions of UserBuffer for supported types
template class UserBuffer<LPVOID>;
template class UserBuffer<LPDWORD>;
template class UserBuffer<LPOVERLAPPED>;

OverlappedUserBuffer::OverlappedUserBuffer(LPOVERLAPPED lpOverlapped)
: UserBuffer<LPOVERLAPPED>(UBA_WRITE | UBA_ASYNC, lpOverlapped)
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
	SetEvent(mhEvent);
	Flush();
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