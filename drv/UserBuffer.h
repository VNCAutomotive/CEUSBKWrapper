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

// UserBuffer.h : Represents a user buffer

#ifndef USER_BUFFER_H
#define USER_BUFFER_H


#define UBA_READ            0x1
#define UBA_WRITE           0x2
#define UBA_ASYNC           0x4
#define UBA_FORCE_DUPLICATE 0x8

#define UBA_READ_WRITE      (UBA_READ | UBA_WRITE)


// Templated class only valid for TYPE with defined
// types in UserBuffer.cpp
template <typename T>
class UserBuffer {
public:
	UserBuffer(
		DWORD dwAccessFlags,
		T lpSrcUnmarshalled,
		DWORD dwSize = sizeof(*(reinterpret_cast<T>(1))));
	~UserBuffer();
	BOOL Valid() const;
	T UserPtr();
	T Ptr();
	BOOL Flush();
	DWORD Size();
private:
	static DWORD ArgDescFromAccessFlags(DWORD dwAccessFlags);
private:
	const LPVOID mlpSrcUnmarshalled;
	const DWORD mSize;
	const BOOL mAsync;
	const DWORD mArgDesc;
	LPVOID mlpSyncMarshalled;
	LPVOID mlpAsyncMarshalled;
};

class OverlappedUserBuffer : public UserBuffer<LPOVERLAPPED> {
public:
	OverlappedUserBuffer(LPOVERLAPPED lpOverlapped);
	~OverlappedUserBuffer();
	OVERLAPPED& operator->();
	BOOL Completed() const;
	void Complete(DWORD dwStatus, DWORD dwBytesTransferred);
	void Abort();
private:
	HANDLE mhEvent;
	BOOL mCompleted;

};

#endif // USER_BUFFER_H