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

// ControlTransfer.h : Represents a pending control transfer

#ifndef CONTROL_TRANSFER_H
#define CONTROL_TRANSFER_H

#include "ceusbkwrapper_common.h"
#include "Transfer.h"
#include "DevicePtr.h"
#include "UserBuffer.h"

class OpenContext;
class UsbDeviceList;

class ControlTransfer : public Transfer {
public:
	ControlTransfer(
		OpenContext* OpenContext,
		DevicePtr& device,
		LPUKWD_CONTROL_TRANSFER_INFO lpTransferInfo);
	virtual ~ControlTransfer();
	BOOL Start();
private:
	UKWD_CONTROL_TRANSFER_INFO mTransferInfo;
};


#endif // CONTROL_TRANSFER_H