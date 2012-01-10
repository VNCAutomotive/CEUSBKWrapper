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

// TransferPtr.h : RAII wrapper for Transfer

#ifndef TRANSFERPTR_H
#define TRANSFERPTR_H

class OpenContext;
class TransferList;
class Transfer;

class TransferPtr {
public:
	TransferPtr(OpenContext* OpenContext, LPOVERLAPPED lpOverlapped);
	TransferPtr(const TransferPtr& device);
	BOOL Valid() const;
	Transfer* operator->();
	Transfer* Get();
	~TransferPtr();
private:
	TransferList* mList;
	Transfer* mTransfer;
};

#endif // TRANSFERPTR_H