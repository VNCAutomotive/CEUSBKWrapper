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

#ifndef READWRITEMUTEX_H
#define READWRITEMUTEX_H

/*
 * This MRSW lock has the following behaviour:
 *  - One or more writer attempting to lock prevent any further readers from
 *    aquiring the lock, as long as they beat the read lock to getting mTryingToLock.
 *  - Writers waiting to lock for write have priority over reads, assuming they win
 *    the race to increment mWaitingWriters.
 */
class ReadWriteMutex {
public:
	ReadWriteMutex();
	~ReadWriteMutex();
	BOOL Init();
	void ReadLock();
	void ReadUnlock();
	void WriteLock();
	void WriteUnlock();
private:
	HANDLE mMutex;
	HANDLE mCanWriteLockEvent;
	HANDLE mCanReadLockEvent;
	DWORD mReaderCount;
	DWORD mWaitingWriters;
};

class ReadLocker {
public:
	ReadLocker(ReadWriteMutex& aMutex);
	~ReadLocker();
public:
	void unlock();
	void relock();
private:
	ReadWriteMutex& mMutex;
	bool mLocked;
};

class WriteLocker {
public:
	WriteLocker(ReadWriteMutex& aMutex);
	~WriteLocker();
public:
	void unlock();
	void relock();
private:
	ReadWriteMutex& mMutex;
	bool mLocked;
};

#endif // MUTEXLOCKER_H