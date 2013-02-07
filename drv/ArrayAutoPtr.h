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

// ArrayAutoPtr.h : Basic autoptr wrapper for arrays allocated with new[].

#ifndef ARRAYAUTOPTR_H
#define ARRAYAUTOPTR_H

template <typename T> class ArrayAutoPtr {
public:
	ArrayAutoPtr(T* ptr=NULL);
	ArrayAutoPtr(ArrayAutoPtr<T>& autoPtr);
	~ArrayAutoPtr();

	ArrayAutoPtr<T>& operator=(T* ptr);
	ArrayAutoPtr<T>& operator=(ArrayAutoPtr<T>& autoPtr);

	T* Get();
	const T& Get(size_t i);
	void Release();	

	void Set(size_t i, const T& value);

private:
	T* mPtr;

};

template <typename T>
ArrayAutoPtr<T>::ArrayAutoPtr(T* ptr)
	: mPtr(ptr)
{
}

template <typename T>
ArrayAutoPtr<T>::ArrayAutoPtr(ArrayAutoPtr<T>& autoPtr)
	: mPtr(autoPtr.mPtr)
{
	autPtr.Release();
}

template <typename T>
ArrayAutoPtr<T>::~ArrayAutoPtr()
{
	delete[] mPtr;
}

template <typename T>
ArrayAutoPtr<T>& ArrayAutoPtr<T>::operator=(T* ptr)
{
	mPtr = ptr;
	return *this;
}

template <typename T>
ArrayAutoPtr<T>& ArrayAutoPtr<T>::operator=(ArrayAutoPtr& autoPtr)
{
	mPtr = autoPtr.mPtr;
	autoPtr.Release();
	return *this;
}

template <typename T>
typename T* ArrayAutoPtr<T>::Get()
{
	return mPtr;
}

template <typename T>
typename const T& ArrayAutoPtr<T>::Get(size_t i)
{
	return mPtr[i];
}

template <typename T>
void ArrayAutoPtr<T>::Release()
{
	mPtr = NULL;
}

template <typename T>
void ArrayAutoPtr<T>::Set(size_t i, const T& value)
{
	mPtr[i] = value;
}


#endif // ARRAYAUTOPTR_H