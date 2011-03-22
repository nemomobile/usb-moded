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

#include <dbus/dbus.h>

#define USB_MODE_SERVICE	"com.meego.usb_moded"
#define USB_MODE_INTERFACE	"com.meego.usb_moded"
#define USB_MODE_OBJECT		"/com/meego/usb_moded"

/* supported methods */
#define USB_MODE_APP_STATE	"ready"
