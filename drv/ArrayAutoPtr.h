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

// ArrayAutoPtr.h : Basic autoptr wrapper for arrays allocated with new[].

#ifndef ARRAYAUTOPTR_H
#define ARRAYAUTOPTR_H

template <typename T> class ArrayAutoPtr {
public:
	ArrayAutoPtr(T* ptr=NULL);
	~ArrayAutoPtr();

	T* Get();
	T& Get(size_t i);
	void Release();	

private:
	T* mPtr;

};

template <typename T>
ArrayAutoPtr<T>::ArrayAutoPtr(T* ptr)
	: mPtr(ptr)
{
}

template <typename T>
ArrayAutoPtr<T>::~ArrayAutoPtr()
{
	delete[] mPtr;
}

template <typename T>
typename T* ArrayAutoPtr<T>::Get()
{
	return mPtr;
}

template <typename T>
typename T& ArrayAutoPtr<T>::Get(size_t i)
{
	return mPtr[i];
}

template <typename T>
ArrayAutoPtr<T>::Release()
{
	mPtr = NULL;
}

#endif // ARRAYAUTOPTR_H