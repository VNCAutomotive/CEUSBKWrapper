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

// librarydbg.h : Defines debug zones for library debug output

#ifndef LIBRARYDBG_H
#define LIBRARYDBG_H

#define DBG_ERROR           1
#define DBG_WARNING         2
#define DBG_ENTRYPOINT      4
#define DBG_FUNCTION        8

#define ZONE_ERROR       DEBUGZONE(0)
#define ZONE_WARNING     DEBUGZONE(1)
#define ZONE_ENTRYPOINT  DEBUGZONE(2)
#define ZONE_FUNCTION    DEBUGZONE(3)

#define ERROR_MSG(a)         RETAILMSG(ZONE_ERROR, a)
#define WARN_MSG(a)          RETAILMSG(ZONE_WARNING, a)
#define ENTRYPOINT_MSG(a)    DEBUGMSG(ZONE_ENTRYPOINT, a)
#define FUNCTION_MSG(a)      DEBUGMSG(ZONE_FUNCTION, a)

extern DBGPARAM dpCurSettings;

#endif // LIBRARYDBG_H