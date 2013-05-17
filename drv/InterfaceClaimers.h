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

// InterfaceClaimers.h: Maintains a list of contexts which have claimed an interface and the reference count for
// how many times they've claimed it

#ifndef INTERFACECLAIMERS_H
#define INTERFACECLAIMERS_H

class UsbDevice;

class InterfaceClaimers {
public:
	InterfaceClaimers();
	~InterfaceClaimers();
	BOOL Init(const USB_INTERFACE& iface);
	BOOL AnyClaimed() const;
	DWORD InterfaceValue() const;
	void SetClaimable(BOOL claimable);
	BOOL IsClaimable() const;
	BOOL IsClaimed(LPVOID Context) const;
	BOOL Claim(LPVOID Context, BOOL& IsFirst);
	BOOL Release(LPVOID Context);
	void ReleaseAll(LPVOID Context);
	BOOL HasEndpoint(UCHAR Endpoint) const;
	USB_PIPE GetPipeForEndpoint(UCHAR Endpoint) const;
	USB_PIPE GetPipeForIndex(DWORD dwIndex) const;
	DWORD GetPipeCount() const;
	BOOL SetPipeForEndpoint(UCHAR Endpoint, USB_PIPE UsbPipe);
	BOOL SetPipeForIndex(DWORD Index, USB_PIPE UsbPipe);
private:
	BOOL ResizeContexts(DWORD mContextLength);
	void RemoveEmptyContexts();
private:
	struct ClaimedContext {
		LPVOID Context;
		DWORD Count;
	};
	UsbDevice* mDevice;
	DWORD mInterfaceValue;
	ClaimedContext* mContexts;
	DWORD mContextsCount; // Used length of mContexts
	DWORD mContextsLength; // Allocated length of mContexts
	BOOL mClaimable;
	struct Endpoints {
		UCHAR Address;
		USB_PIPE Pipe;
	};
	Endpoints* mEndpoints; // Addresses of endpoints in this interface
	DWORD mEndpointsCount;
};

#endif //INTERFACECLAIMERS_H