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

// ceusbkwrapperdrv.h: Defines the exported entry points for the
// driver's stream interface.

#ifndef CEUSBKWRAPPERDRV_H
#define CEUSBKWRAPPERDRV_H

#include <StdAfx.h>

#ifdef __cplusplus
extern "C" {
#endif

// USB driver interface methods USBInstallDriver, USBDeviceAttach
// and USBUnInstallDriver come from headers included by StdAfx.h

// Stream driver interface files
DWORD Init(
  LPCTSTR pContext,
  DWORD dwBusContext);

BOOL Deinit(
  DWORD hDeviceContext);

DWORD Open(
  DWORD hDeviceContext,
  DWORD AccessCode,
  DWORD ShareMode);

BOOL Close(
  DWORD hOpenContext);

DWORD Read(
  DWORD hOpenContext,
  LPVOID pBuffer,
  DWORD Count);

DWORD Write(
  DWORD hOpenContext,
  LPVOID pBuffer,
  DWORD Count);

DWORD Seek(
  DWORD hOpenContext,
  long Amount,
  WORD Type);

BOOL IOControl(
  DWORD hOpenContext,
  DWORD dwCode,
  PBYTE pBufIn,
  DWORD dwLenIn,
  PBYTE pBufOut,
  DWORD dwLenOut,
  PDWORD pdwActualOut);

void PowerUp(
  DWORD hDeviceContext);

void PowerDown(
  DWORD hDeviceContext);

BOOL PreClose(
  DWORD hOpenContext);

BOOL PreDeinit(
  DWORD hDeviceContext);

#ifdef __cplusplus
};
#endif

#endif // CEUSBKWRAPPERDRV_H