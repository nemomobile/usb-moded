/*
  Copyright (C) 2010 Nokia Corporation. All rights reserved.

  Author: Philippe De Swert <philippe.de-swert@nokia.com>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the Lesser GNU General Public License 
  version 2 as published by the Free Software Foundation. 

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.
 
  You should have received a copy of the Lesser GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
  02110-1301 USA
*/
/*
 * Interacts with the devicelock to know if we can expose the system contents or not
 */

/*============================================================================= */

#define DEVICELOCK_SERVICE		"com.nokia.devicelock"
#define DEVICELOCK_REQUEST_PATH		"/request"
#define DEVICELOCK_REQUEST_IF		"com.nokia.devicelock"
#define DEVICELOCK_STATE_REQ		"getState"

#define DEVICELOCK_LOCKED		"Locked"

#define MATCH_DEVICELOCK_SIGNALS\
  "type='signal'"\
  ",interface='"DEVICELOCK_REQUEST_IF"'"\
  ",path='"DEVICELOCK_REQUEST_PATH"'"

