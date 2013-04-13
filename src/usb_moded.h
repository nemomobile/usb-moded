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

#ifndef USB_MODED_H_
#define USB_MODED_H_

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <config.h>
#include <fcntl.h>

#include <sys/stat.h>
#include <sys/wait.h>

#include <glib-2.0/glib.h>
#include <glib-object.h>

#define USB_MODED_LOCKFILE	"/var/run/usb_moded.pid"
#define MAX_READ_BUF 512

/** 
 * a struct containing all the usb_moded info needed 
 */
typedef struct usb_mode  
{
  /*@{*/
  gboolean connected; 	/* connection status, 1 for connected */
  gboolean mounted;  	/* mount status, 1 for mounted -UNUSED atm- */
  char *mode;  		/* the mode name */
  char *module; 	/* the module name for the specific mode */
  /*@}*/
}usb_mode;

void set_usb_connected(gboolean connected);
void set_usb_connected_state(void);
void set_usb_mode(const char *mode);
const char * get_usb_mode(void);
void set_usb_module(const char *module);
const char * get_usb_module(void);
gboolean get_usb_connection_state(void);
void set_usb_connection_state(gboolean state);

#endif /* USB_MODED_H */
