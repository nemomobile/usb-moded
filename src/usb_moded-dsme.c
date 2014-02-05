/**
  @file usb_moded-dsme.c

  Copyright (C) 2013 Jolla. All rights reserved.

  @author: Philippe De Swert <philippe.deswert@jollamobile.com>
  @author: Jonni Rainisto <jonni.rainisto@jollamobile.com>

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
#include <dbus/dbus-glib-lowlevel.h>

#include "usb_moded-dsme.h"
#include "usb_moded-log.h"

/** Checks if the device is is USER-state.
 *
 * @return 1 if it is in USER-state, 0 for not
 *
 */
int is_in_user_state(void)
{
  DBusConnection *dbus_conn = NULL;
  DBusMessage *msg = NULL, *reply = NULL;
  DBusError error;
  int ret = 0;
  char* buffer = NULL;

  dbus_error_init(&error);

  if( (dbus_conn = dbus_bus_get(DBUS_BUS_SYSTEM, &error)) == 0 )
  {
     log_err("Could not connect to dbus systembus (is_in_user_state)\n");
     /* dbus system bus is broken or not there, so assume not in USER state */
     return(ret);
  }

  if ((msg = dbus_message_new_method_call("com.nokia.dsme", "/request", "com.nokia.dsme.request", "get_state")) != NULL)
  {
    if ((reply = dbus_connection_send_with_reply_and_block(dbus_conn, msg, -1, NULL)) != NULL)
    {
       dbus_message_get_args(reply, &error, DBUS_TYPE_STRING, &buffer, DBUS_TYPE_INVALID);
       dbus_message_unref(reply);
    }
     dbus_message_unref(msg);
  }
  dbus_connection_unref(dbus_conn);

  if(buffer)
  {
	log_debug("user state = %s\n", buffer);
	if (strcmp(buffer, "USER")==0) ret = 1;
  }
  return(ret);
}

