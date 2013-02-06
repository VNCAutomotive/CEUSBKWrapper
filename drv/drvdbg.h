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

// drvdbg.h : Defines debug zones for driver debug output

#ifndef DRVDBG_H
#define DRVDBG_H

#define DBG_ERROR             1
#define DBG_WARNING           2
#define DBG_ENTRYPOINT        4
#define DBG_FUNCTION          8
#define DBG_DEVLIFETIME      16
#define DBG_TRANSFERLIFETIME 32
#define DBG_DISCOVERY        64
#define DBG_IFACEFILTER     128

#define ZONE_ERROR       DEBUGZONE(0)
#define ZONE_WARNING     DEBUGZONE(1)
#define ZONE_ENTRYPOINT  DEBUGZONE(2)
#define ZONE_FUNCTION    DEBUGZONE(3)
#define ZONE_DEVLIFETIME DEBUGZONE(4)
#define ZONE_TRANSFERLIFETIME DEBUGZONE(5)
#define ZONE_DISCOVERY        DEBUGZONE(6)
#define ZONE_IFACEFILTER      DEBUGZONE(7)

#define ERROR_MSG(a)            RETAILMSG(ZONE_ERROR, a)
#define WARN_MSG(a)             RETAILMSG(ZONE_WARNING, a)
#define ENTRYPOINT_MSG(a)       DEBUGMSG(ZONE_ENTRYPOINT, a)
#define FUNCTION_MSG(a)         DEBUGMSG(ZONE_FUNCTION, a)
#define DEVLIFETIME_MSG(a)      DEBUGMSG(ZONE_DEVLIFETIME, a)
#define TRANSFERLIFETIME_MSG(a) DEBUGMSG(ZONE_TRANSFERLIFETIME, a)
#define DISCOVERY_MSG(a)        RETAILMSG(ZONE_DISCOVERY, a)
#define IFACEFILTER_MSG(a)      RETAILMSG(ZONE_IFACEFILTER, a);

extern DBGPARAM dpCurSettings;

#endif // DRVDBG_H