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

// ReadWriteMutex.h : Multiple readers and single writer mutex.

#include "StdAfx.h"
#include "ReadWriteMutex.h"

ReadWriteMutex::ReadWriteMutex()
: mMutex(NULL)
, mCanWriteLockEvent(NULL)
, mCanReadLockEvent(NULL)
, mReaderCount(0)
, mWaitingWriters(0)
{
	
}

ReadWriteMutex::~ReadWriteMutex()
{
	if (mMutex)
		CloseHandle(mMutex);
	if (mCanReadLockEvent)
		CloseHandle(mCanReadLockEvent);
	if (mCanWriteLockEvent)
		CloseHandle(mCanWriteLockEvent);
}

BOOL ReadWriteMutex::Init()
{
	mMutex = CreateMutex(NULL, FALSE, NULL);
	mCanReadLockEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	mCanWriteLockEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	return mMutex != NULL && mCanReadLockEvent != NULL && mCanWriteLockEvent != NULL;
}

void ReadWriteMutex::ReadLock()
{
	WaitForSingleObject(mMutex, INFINITE);
	while(mWaitingWriters > 0)
	{
		ResetEvent(mCanReadLockEvent);
		ReleaseMutex(mMutex);
		WaitForSingleObject(mCanReadLockEvent, INFINITE);
		WaitForSingleObject(mMutex, INFINITE);
	}
	++mReaderCount;
	if (mReaderCount == 1)
		// Stop any threads waiting to write from waking up unnecessarily
		ResetEvent(mCanWriteLockEvent);
	SetEvent(mCanReadLockEvent);
	ReleaseMutex(mMutex);
}

void ReadWriteMutex::ReadUnlock()
{
	WaitForSingleObject(mMutex, INFINITE);
	--mReaderCount;
	if (mWaitingWriters == 0)
		// Wake up any threads attempting to read
		SetEvent(mCanReadLockEvent);
	if (mReaderCount == 0)
		// Wake up any threads attempting to write
		SetEvent(mCanWriteLockEvent);
	ReleaseMutex(mMutex);
}

void ReadWriteMutex::WriteLock()
{
	WaitForSingleObject(mMutex, INFINITE);
	++mWaitingWriters;
	if (mReaderCount > 0)
	{
		ResetEvent(mCanWriteLockEvent);
		ReleaseMutex(mMutex);
		WaitForSingleObject(mCanWriteLockEvent, INFINITE);
		WaitForSingleObject(mMutex, INFINITE);
	}
	--mWaitingWriters;
	// Leave mMutex locked as it's used to mean a write lock
}

void ReadWriteMutex::WriteUnlock()
{
	if (mWaitingWriters == 0)
		SetEvent(mCanReadLockEvent);
	SetEvent(mCanWriteLockEvent);
	ReleaseMutex(mMutex);
}

ReadLocker::ReadLocker(ReadWriteMutex& aMutex)
: mMutex(aMutex),
  mLocked(true)
{
	mMutex.ReadLock();
}

ReadLocker::~ReadLocker()
{
	unlock();
}

void ReadLocker::unlock()
{
	if (mLocked)
	{
		mMutex.ReadUnlock();
		mLocked = false;
	}
}

void ReadLocker::relock()
{
	if (!mLocked)
	{
		mMutex.ReadLock();
		mLocked = true;
	}
}

WriteLocker::WriteLocker(ReadWriteMutex& aMutex)
: mMutex(aMutex),
  mLocked(true)
{
	mMutex.WriteLock();
}

WriteLocker::~WriteLocker()
{
	unlock();
}

void WriteLocker::unlock()
{
	if (mLocked)
	{
		mMutex.WriteUnlock();
		mLocked = false;
	}
}

void WriteLocker::relock()
{
	if (!mLocked)
	{
		mMutex.WriteLock();
		mLocked = true;
	}
}