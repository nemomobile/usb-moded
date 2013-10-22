/**
 
  @file: usb_moded-devicelock.c
   
  Copyright (C) 2010 Nokia Corporation. All rights reserved.

  @author: Philippe De Swert <philippe.de-swert@nokia.com>

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

#include <stdlib.h>
#include <string.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib-lowlevel.h>

#include "usb_moded-devicelock.h"
#include "usb_moded-lock.h"
#include "usb_moded-log.h"
#include "usb_moded.h"
#include "usb_moded-modes.h"

static DBusHandlerResult devicelock_unlocked_cb(DBusConnection *conn, DBusMessage *msg, void *user_data);

DBusConnection *dbus_conn_devicelock = NULL;
	
/** Checks if the device is locked.
 * 
 * @return 0 for unlocked, 1 for locked
 *
 */
int usb_moded_get_export_permission(void)
{
  DBusConnection *dbus_conn_devicelock = NULL;
  DBusMessage *msg = NULL, *reply = NULL;
  DBusError error;
  int ret = 2;

  dbus_error_init(&error);

  if( (dbus_conn_devicelock = dbus_bus_get(DBUS_BUS_SYSTEM, &error)) == 0 )
  {
	 log_err("Could not connect to dbus for devicelock\n"); 
  }

  if ((msg = dbus_message_new_method_call(DEVICELOCK_SERVICE, DEVICELOCK_REQUEST_PATH, DEVICELOCK_REQUEST_IF, DEVICELOCK_STATE_REQ)) != NULL) 
  {
     /* default dbus timeout is too long, timeout after 1,5 seconds */
     if ((reply = dbus_connection_send_with_reply_and_block(dbus_conn_devicelock, msg, 1500, NULL)) != NULL)
     {
       dbus_message_get_args(reply, NULL, DBUS_TYPE_INT32, &ret, DBUS_TYPE_INVALID);
       dbus_message_unref(reply);
     }
     dbus_message_unref(msg);
  } 
  dbus_connection_unref(dbus_conn_devicelock);
  
  log_debug("devicelock state = %d\n", ret);
  return(ret);
}

int start_devicelock_listener(void)
{
  DBusError       err = DBUS_ERROR_INIT;

  if( (dbus_conn_devicelock = dbus_bus_get(DBUS_BUS_SYSTEM, &err)) == 0 )
  {
	 log_err("Could not connect to dbus for devicelock\n"); 
	 goto cleanup;
  }

  dbus_bus_add_match(dbus_conn_devicelock, MATCH_DEVICELOCK_SIGNALS, &err);
  if( dbus_error_is_set(&err) )
  {
    goto cleanup;
  }
  if( !dbus_connection_add_filter(dbus_conn_devicelock, devicelock_unlocked_cb , 0, 0) )
  {
        log_err("adding system dbus filter for devicelock failed");
    goto cleanup;
  }
  //dbus_connection_setup_with_g_main(dbus_conn_devicelock, NULL);

cleanup:
  dbus_error_free(&err);
  return(1);
}

int stop_devicelock_listener(void)
{
  if(dbus_conn_devicelock)
  {
	dbus_connection_remove_filter(dbus_conn_devicelock, devicelock_unlocked_cb, NULL);
	dbus_bus_remove_match(dbus_conn_devicelock, MATCH_DEVICELOCK_SIGNALS, NULL);
	dbus_connection_unref(dbus_conn_devicelock);
  }

  return 0;
}

static DBusHandlerResult devicelock_unlocked_cb(DBusConnection *conn, DBusMessage *msg, void *user_data)
{
  DBusHandlerResult   result    = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  const char         *interface = dbus_message_get_interface(msg);
  const char         *member    = dbus_message_get_member(msg);
  const char         *object    = dbus_message_get_path(msg);
  int                 type      = dbus_message_get_type(msg);
  int ret=0;

  (void) user_data;

  // sanity checks
  if( !interface || !member || !object ) goto cleanup;
  if( type != DBUS_MESSAGE_TYPE_SIGNAL ) goto cleanup;
  if( strcmp(interface, DEVICELOCK_REQUEST_IF) ) goto cleanup;
  if( strcmp(object, DEVICELOCK_REQUEST_PATH) )  goto cleanup;

  // handle known signals
  else if( !strcmp(member, "stateChanged") )
  {
        dbus_message_get_args(msg, NULL, DBUS_TYPE_INT32, &ret, DBUS_TYPE_INVALID);
    log_debug("Devicelock state changed. New state = %d\n", ret);
  	if(ret == 0 && get_usb_connection_state() == 1 )
  	{	
        log_debug("usb_mode %s\n", get_usb_mode());
            if(!strcmp(get_usb_mode(), MODE_UNDEFINED) || !strcmp(get_usb_mode(), MODE_CHARGING)) {
            log_debug("set_usb");
			set_usb_connected_state();
        }
  	}
  }
  result = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

cleanup:
  return result;
}

