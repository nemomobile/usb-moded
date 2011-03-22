/**
  @file usb_moded-chargerdetection.c

  Copyright (C) 2010 Nokia Corporation. All rights reserved.

  @author: Philippe De Swert <philippe.de-swert@nokia.com>
  	  Simo Piiroinen <simo.piiroinen@nokia.com>
  	  Tuomo Tanskanen <ext-tuomo.1.tanskanen@nokia.com>

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

/*============================================================================= */

#include <sys/time.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#include <glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include "usb_moded.h"
#include "usb_moded-hw-ab.h"
#include "usb_moded-log.h"

/* ========================================================================= *
 * Quick guide to the state logic ...
 *
 * startup:
 * @ hwal_init()
 * - connect to system bus
 *
 * hwaö_dbus_message_cb
 * @ hwal_set_usb_connected()
 * - broadcast connection status over dbus
 *
 * shutdown:
 * @ hwal_cleanup()
 * - forget dbus connection
 *
 * dbus disconnect:
 * - exit
 * ========================================================================= */

/* ========================================================================= *
 * CONSTANTS
 * ========================================================================= */

#define BME_INTERFACE_DBUS "com.nokia.bme.signal"
#define BME_PATH_DBUS "/com/nokia/bme/signal"
#define BME_MEMBER_CHARGER_CONNECTED "charger_connected"
#define BME_MEMBER_CHARGER_DISCONNECTED "charger_disconnected"

// D-Bus signal match for tracking bme signal on charger connections
#define MATCH_BME_CHARGER_CONNECTED\
    "type='signal'"\
    ",interface='"BME_INTERFACE_DBUS"'"\
    ",path='"BME_PATH_DBUS"'"\
    ",member='"BME_MEMBER_CHARGER_CONNECTED"'"
	
// D-Bus signal match for tracking bme signal on charger disconnections
#define MATCH_BME_CHARGER_DISCONNECTED\
    "type='signal'"\
    ",interface='"BME_INTERFACE_DBUS"'"\
    ",path='"BME_PATH_DBUS"'"\
    ",member='"BME_MEMBER_CHARGER_DISCONNECTED"'"

// D-Bus signal match to catch DESKTOP_VISIBLE in case of act_dead->user transition
#define STARTUP_INTERFACE_DBUS "com.nokia.startup.signal"
#define STARTUP_PATH_DBUS "/com/nokia/startup/signal"
#define STARTUP_MEMBER_DESKTOP_VISIBLE "desktop_visible"

// D-Bus signal match for desktop visible signal
#define MATCH_STARTUP_DESKTOP_VISIBLE\
    "type='signal'"\
    ",interface='"STARTUP_INTERFACE_DBUS"'"\
    ",path='"STARTUP_PATH_DBUS"'"\
    ",member='"STARTUP_MEMBER_DESKTOP_VISIBLE"'"

// id's meaning usb host connection in /sys/class/power_supply/usb/uevent"
#define USB_UEVENT_FILE "/sys/class/power_supply/usb/uevent"
#define USB_TYPE1 "POWER_SUPPLY_TYPE=USB_CDP"
#define USB_TYPE2 "POWER_SUPPLY_TYPE=USB\n"
#define USB_PRESENT "POWER_SUPPLY_PRESENT=1"


/* ========================================================================= *
 * STATE VARIABLES
 * ========================================================================= */

static DBusConnection *hwal_dbus_con  = 0; // system bus connection


/* ========================================================================= *
 * STATE LOGIC FUNCTIONS
 * ========================================================================= */

/* ------------------------------------------------------------------------- *
 * hwal_is_usb_host_connected --  get charger type from sysfs, triggered by
 *                                bme dbus signal about charger connection
 * ------------------------------------------------------------------------- */

static gboolean hwal_is_usb_host_connected(void)
{
  gboolean is_usb = TRUE;
  gchar *file_content = NULL;
  gsize file_length = 0;

  // read uevent file contents, check for keywords and see if its host usb
  if (g_file_get_contents(USB_UEVENT_FILE, &file_content, &file_length, NULL) == TRUE)
  {
	gboolean usb_type1_found = FALSE, usb_type2_found = FALSE;
	gboolean usb_cable_present = FALSE;

    usb_type1_found = (g_strrstr(file_content, USB_TYPE1) != NULL);
    usb_type2_found = (g_strrstr(file_content, USB_TYPE2) != NULL);
    usb_cable_present = (g_strrstr(file_content, USB_PRESENT) != NULL);
   
    log_debug("%s: type1=%d, type2=%d, present=%d\n",
      __FUNCTION__, usb_type1_found, usb_type2_found, usb_cable_present);
	g_free(file_content);
	
	is_usb = usb_cable_present & (usb_type1_found | usb_type2_found);
  }

  /* fallback value is TRUE, so in case of errors we rather show dialog
   * than keep silent, thus disabling usb practically */
  return is_usb;
}

/* ------------------------------------------------------------------------- *
 * hwal_set_usb_connected  --  forward usb connection status
 * ------------------------------------------------------------------------- */

static void hwal_set_usb_connected(gboolean connected)
{
  log_notice("%s(%s)\n", __FUNCTION__, connected ? "CONNECTED" : "DISCONNECTED");
  set_usb_connected(connected);
}


/* ========================================================================= *
 * CALLBACK FUNCTIONS
 * ========================================================================= */

/* ------------------------------------------------------------------------- *
 * hwal_dbus_message_cb  -- handle dbus message
 * ------------------------------------------------------------------------- */

static
DBusHandlerResult
hwal_dbus_message_cb(DBusConnection *conn,
		     DBusMessage *msg,
		     void *user_data)
{
  DBusHandlerResult   result    = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  const char         *interface = dbus_message_get_interface(msg);
  const char         *member    = dbus_message_get_member(msg);
  const char         *object    = dbus_message_get_path(msg);
  int                 type      = dbus_message_get_type(msg);
  const char         *typestr   = dbus_message_type_to_string(type);
  DBusError   err  = DBUS_ERROR_INIT;

  if( !interface || !member || !object )
  {
    goto cleanup;
  }

  log_debug("DBUS: %s - %s.%s(%s)\n", typestr, interface, member, object);

  if( type == DBUS_MESSAGE_TYPE_SIGNAL )
  {
    if( !strcmp(interface, BME_INTERFACE_DBUS) && !strcmp(object, BME_PATH_DBUS) )
    {
        if( (!strcmp(member, BME_MEMBER_CHARGER_CONNECTED) ) )
        {
          log_debug("bme signals charger connected\n");
	  if(hwal_is_usb_host_connected())
		  hwal_set_usb_connected(TRUE);
	}
	else if( !strcmp(member, BME_MEMBER_CHARGER_DISCONNECTED) )
	{
          log_debug("bme signals charger disconnected\n");
          hwal_set_usb_connected(FALSE);
	}
     }		
  }

cleanup:

  dbus_error_free(&err);
  return result;
}

/* ------------------------------------------------------------------------- *
 * hwal_init  --  set up the hw abstraction layer
 * ------------------------------------------------------------------------- */

gboolean hwal_init(void)
{
  gboolean    res = FALSE;
  DBusError   err = DBUS_ERROR_INIT;

  // initialize system bus connection

  if( (hwal_dbus_con = dbus_bus_get(DBUS_BUS_SYSTEM, &err)) == 0 )
  {
    log_crit("%s: %s: %s\n", "bus get", err.name, err.message);
    goto cleanup;
  }

  if( !dbus_connection_add_filter(hwal_dbus_con, hwal_dbus_message_cb, 0, 0) )
  {
    goto cleanup;
  }

  dbus_bus_add_match(hwal_dbus_con, MATCH_BME_CHARGER_CONNECTED, &err);
  if( dbus_error_is_set(&err) )
  {
    log_crit("%s: %s: %s\n", "add match", err.name, err.message);
    goto cleanup;
  }
  dbus_bus_add_match(hwal_dbus_con, MATCH_BME_CHARGER_DISCONNECTED, &err);
  if( dbus_error_is_set(&err) )
  {
    log_crit("%s: %s: %s\n", "add match", err.name, err.message);
    goto cleanup;
  }

  dbus_connection_set_exit_on_disconnect(hwal_dbus_con, TRUE);

  dbus_connection_setup_with_g_main(hwal_dbus_con, NULL);

  if(hwal_is_usb_host_connected())
	  hwal_set_usb_connected(TRUE);

  res = TRUE;

cleanup:

  dbus_error_free(&err);
  return res;
}

/* ------------------------------------------------------------------------- *
 * hwal_cleanup  --  cleans up the hw abstraction layer on exit
 * ------------------------------------------------------------------------- */

void hwal_cleanup(void)
{
  // drop system bus connection
  if( hwal_dbus_con != 0 )
  {
    DBusError err = DBUS_ERROR_INIT;

    dbus_bus_remove_match(hwal_dbus_con, MATCH_BME_CHARGER_CONNECTED, &err);
    if( dbus_error_is_set(&err) )
    {
      log_err("%s: %s: %s\n", "remove match", err.name, err.message);
      dbus_error_free(&err);
    }
    dbus_bus_remove_match(hwal_dbus_con, MATCH_BME_CHARGER_DISCONNECTED, &err);
    if( dbus_error_is_set(&err) )
    {
      log_err("%s: %s: %s\n", "remove match", err.name, err.message);
      dbus_error_free(&err);
    }

    // remove message handlers
    dbus_connection_remove_filter(hwal_dbus_con, hwal_dbus_message_cb, 0);

    // forget the connection
    dbus_connection_unref(hwal_dbus_con);
    hwal_dbus_con = 0;

    // TODO: detach from the mainloop, how?
  }
}
