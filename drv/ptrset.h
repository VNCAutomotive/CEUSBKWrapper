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

// ptrarray.h: Simple templated implementation of a sorted array class

#ifndef PTRARRAY_H
#define PTRARRAY_H

template <typename T> class PtrArray {
public:
	class iterator {
	public:
		friend class PtrArray<T>;
		BOOL operator==(const iterator& rhs);
		BOOL operator!=(const iterator& rhs);
		iterator& operator++();
		iterator operator++(int);
		T* operator*();
	private:
		iterator(PtrArray<T>& set, DWORD idx);
	private:
		PtrArray<T>& mSet;
		DWORD mIdx;
	};
	friend class iterator;
	typedef iterator const_iterator;
	PtrArray();
	~PtrArray();
	BOOL insert(T* value);
	void erase(T* value);
	BOOL empty() const;
	iterator begin();
	iterator end();
	iterator find(T* value);
private:
	BOOL resize(DWORD newSize);
	DWORD findIdx(T* value);
private:
	T** mValues; // Sorted array of values
	DWORD mValuesSize; // Length of mValues
	DWORD mValuesCount; // Number of valid values in mValues
};



template <typename T>
BOOL PtrArray<T>::iterator::operator==(const iterator& rhs)
{
	return ((&mSet) == (&rhs.mSet)) && mIdx == rhs.mIdx;
}

template <typename T>
BOOL PtrArray<T>::iterator::operator!=(const iterator& rhs)
{
	return !(*this == rhs);
}

template <typename T>
typename PtrArray<T>::iterator& PtrArray<T>::iterator::operator++()
{
	++mIdx;
	return *this;
}

template <typename T>
typename PtrArray<T>::iterator PtrArray<T>::iterator::operator++(int)
{
	++mIdx;
	return iterator(mSet, mIdx - 1);
}

template <typename T>
typename T* PtrArray<T>::iterator::operator*()
{
	return mSet.mValues[mIdx];
}

template <typename T>
PtrArray<T>::iterator::iterator(PtrArray<T>& set, DWORD idx)
: mSet(set), mIdx(idx)
{
}

template <typename T>
PtrArray<T>::PtrArray()
: mValues(NULL)
, mValuesSize(0)
, mValuesCount(0)
{
}

template <typename T>
PtrArray<T>::~PtrArray()
{
	if (mValues) {
		free(mValues);
	}
}

template <typename T>
BOOL PtrArray<T>::insert(T* value)
{
	// Look for the correct place to insert the value
	DWORD idx = findIdx(value);
	if (idx == mValuesCount) {
		// needs to be added at the end
		if (!resize(mValuesCount + 1)) {
			return FALSE;
		}
		mValues[idx] = value;
		++mValuesCount;
		return TRUE;
	} else {
		// needs to be added at this place
		if (!resize(mValuesCount + 1)) {
			return FALSE;
		}
		// Move the other entries up one
		memmove(mValues + idx + 1, mValues + idx, (mValuesCount - idx) * sizeof(T*));
		mValues[idx] = value;
		++mValuesCount;
		return TRUE;
	}
}

template <typename T>
void PtrArray<T>::erase(T* value)
{
	// Look for the value
	DWORD idx = findIdx(value);
	if (idx == mValuesCount || mValues[idx] != value)
		// not-found
		return;
	// Otherwise move the other values down
	memmove(mValues + idx, mValues + idx + 1, (mValuesCount - (idx + 1)) * sizeof(T*));
	// And possibly resize
	resize(mValuesCount - 1);
	--mValuesCount;
}

template <typename T>
BOOL PtrArray<T>::empty() const
{
	return mValuesCount == 0;
}

template <typename T>
typename PtrArray<T>::iterator PtrArray<T>::begin()
{
	return iterator(*this, 0);
}

template <typename T>
typename PtrArray<T>::iterator PtrArray<T>::end()
{
	return iterator(*this, mValuesCount);
}

template <typename T>
typename PtrArray<T>::iterator PtrArray<T>::find(T* value)
{
	DWORD idx = findIdx(value);
	if (idx == mValuesCount || mValues[idx] != value)
		return end();
	return iterator(*this, idx);
}

template <typename T>
BOOL PtrArray<T>::resize(DWORD newSize)
{
	DWORD reallocSize = mValuesSize;
	if (newSize > mValuesSize) {
		// Need to increase the size
		reallocSize = mValuesSize ? mValuesSize * 2 : 2;
	} else if (newSize < (mValuesSize / 2)) {
		// Need to decrease the size
		reallocSize = mValuesSize / 2;
	}
	if (reallocSize == mValuesSize) {
		// Nothing to do
		return TRUE;
	} else if (reallocSize == 0) {
		free(mValues);
		mValues = NULL;
		return TRUE;
	} else if (mValues == NULL) {
		mValues = static_cast<T**>(malloc(reallocSize * sizeof(T*)));
		if (mValues == NULL) {
			return NULL;
		}
		mValuesSize = reallocSize;
		return TRUE;
	}
	T** newValues = static_cast<T**>(realloc(mValues, reallocSize * sizeof(T*)));
	if (newValues == NULL) {
		return FALSE;
	}
	mValues = newValues;
	mValuesSize = reallocSize;
	return TRUE;
}

template <typename T>
DWORD PtrArray<T>::findIdx(T* value)
{
	if (mValuesCount == 0 || mValues == NULL)
		return 0;
	// Perform a binary search on the values
	DWORD start = 0; // Inclusive
	DWORD end = mValuesCount - 1; // Inclusive
	while (end > start) {
		DWORD pivot = (start + end) / 2;
		if (mValues[pivot] < value)
			start = pivot + 1;
		else
			end = pivot;
	}
	if (start == end && mValues[start] == value)
		// Found directly
		return start;
	else if (start >= mValuesCount)
		// Not found and would need placing at end
		return mValuesCount;
	else if (mValues[start] < value)
		// Not found and needs to be placed after current start
		return start + 1;
	else
		// Not found but start is pointing at where it should be inserted
		return start;
}

#endif // PTRARRAY_H