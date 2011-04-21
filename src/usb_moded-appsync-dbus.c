/**
  @file	usb_moded-dbus.c

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

#include <stdio.h>
#include <string.h>

#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include "usb_moded-dbus.h"
#include "usb_moded-dbus-private.h"
#include "usb_moded.h"
#include "usb_moded-log.h"
#include "usb_moded-modes.h"
#include "usb_moded-gconf-private.h"
#include "usb_moded-appsync.h"
#include "usb_moded-appsync-dbus.h"
#include "usb_moded-appsync-dbus-private.h"

static DBusConnection *dbus_connection_ses  = NULL;  // connection
static gboolean        dbus_connection_name = FALSE; // have name
static gboolean        dbus_connection_disc = FALSE; // got disconnected

static void usb_moded_app_sync_cleanup_connection(void);

static DBusHandlerResult handle_disconnect(DBusConnection *conn, DBusMessage *msg, void *user_data);
static DBusHandlerResult msg_handler(DBusConnection *const connection, DBusMessage *const msg, gpointer const user_data);

/**
 * Handle USB_MODE_INTERFACE method calls
 */

static DBusHandlerResult msg_handler(DBusConnection *const connection, DBusMessage *const msg, gpointer const user_data)
{
  DBusHandlerResult   status    = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  int                 type      = dbus_message_get_type(msg);
  const char         *interface = dbus_message_get_interface(msg);
  const char         *member    = dbus_message_get_member(msg);
  const char         *object    = dbus_message_get_path(msg);

  if(!interface || !member || !object) goto IGNORE;

  if( type == DBUS_MESSAGE_TYPE_METHOD_CALL &&
      !strcmp(interface, USB_MODE_INTERFACE) &&
      !strcmp(object, USB_MODE_OBJECT) )

  {
    DBusMessage *reply = 0;

    status = DBUS_HANDLER_RESULT_HANDLED;

    if(!strcmp(member, USB_MODE_APP_STATE))
    {
      char      *use = 0;
      DBusError  err = DBUS_ERROR_INIT;

      if(!dbus_message_get_args(msg, &err, DBUS_TYPE_STRING, &use, DBUS_TYPE_INVALID))
      {
	// could not parse method call args
	reply = dbus_message_new_error(msg, DBUS_ERROR_INVALID_ARGS, member);
      }
      else if( mark_active(use) < 0 )
      {
	// name could not be marked active
	reply = dbus_message_new_error(msg, DBUS_ERROR_INVALID_ARGS, member);
      }
      else if((reply = dbus_message_new_method_return(msg)))
      {
	// generate normal reply
	dbus_message_append_args (reply, DBUS_TYPE_STRING, &use, DBUS_TYPE_INVALID);
      }
      dbus_error_free(&err);
    }
    else
    {
      /*unknown methods are handled here */
      reply = dbus_message_new_error(msg, DBUS_ERROR_UNKNOWN_METHOD, member);
    }

    if( !dbus_message_get_no_reply(msg) )
    {
      if( !reply )
      {
	// we failed to generate reply above -> generate one
	reply = dbus_message_new_error(msg, DBUS_ERROR_FAILED, member);
      }
      if( !reply || !dbus_connection_send(connection, reply, 0) )
      {
	log_debug("Failed sending reply. Out Of Memory!\n");
      }
    }

    if( reply ) dbus_message_unref(reply);
  }

IGNORE:

  return status;
}

/**
 * Handle disconnect signals
 */
static DBusHandlerResult handle_disconnect(DBusConnection *conn, DBusMessage *msg, void *user_data)
{
  if( dbus_message_is_signal(msg, DBUS_INTERFACE_LOCAL, "Disconnected") )
  {
    log_warning("disconnected from session bus - expecting restart/stop soon\n");
    dbus_connection_disc = TRUE;
    usb_moded_app_sync_cleanup_connection();
  }
  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

/**
 * Detach from session bus
 */
static void usb_moded_app_sync_cleanup_connection(void)
{
  if( dbus_connection_ses != 0 )
  {
    /* Remove message filters */
    dbus_connection_remove_filter(dbus_connection_ses, msg_handler, 0);
    dbus_connection_remove_filter(dbus_connection_ses, handle_disconnect, 0);

    /* Release name, if we can still talk to dbus daemon */
    if( !dbus_connection_disc )
    {
      DBusError error = DBUS_ERROR_INIT;
      dbus_bus_release_name(dbus_connection_ses, USB_MODE_SERVICE, &error);
      dbus_error_free(&error);
    }

    dbus_connection_unref(dbus_connection_ses);
    dbus_connection_ses = NULL;
    //dbus_connection_disc = FALSE;
  }
  log_debug("succesfully cleaned up appsync dbus\n");
}

/**
 * Attach to session bus
 */
gboolean usb_moded_app_sync_init_connection(void)
{
  gboolean  result = FALSE;
  DBusError error  = DBUS_ERROR_INIT;

  if( dbus_connection_ses != 0 )
  {
    result = TRUE;
    goto EXIT;
  }

  if( dbus_connection_disc )
  {
    // we've already observed death of session
    goto EXIT;
  }

  /* Connect to session bus */
  if ((dbus_connection_ses = dbus_bus_get(DBUS_BUS_SESSION, &error)) == NULL)
  {
    log_err("Failed to open connection to session message bus; %s\n",  error.message);
    goto EXIT;
  }

  /* Add disconnect handler */
  dbus_connection_add_filter(dbus_connection_ses, handle_disconnect, 0, 0);

  /* Add method call handler */
  dbus_connection_add_filter(dbus_connection_ses, msg_handler, 0, 0);

  /* Make sure we do not get forced to exit if dbus session dies or stops */
  dbus_connection_set_exit_on_disconnect(dbus_connection_ses, FALSE);

  /* Connect D-Bus to the mainloop */
  dbus_connection_setup_with_g_main(dbus_connection_ses, NULL);

  /* everything went fine */
  result = TRUE;

EXIT:
  dbus_error_free(&error);
  return result;
}

/**
 * Init dbus for usb_moded application synchronisation
 *
 * @return TRUE when everything went ok
 */
gboolean usb_moded_app_sync_init(void)
{
  gboolean status = FALSE;
  DBusError error = DBUS_ERROR_INIT;
  int ret;

  if( !usb_moded_app_sync_init_connection() )
  {
    goto EXIT;
  }

  /* Acquire D-Bus service name */
  ret = dbus_bus_request_name(dbus_connection_ses, USB_MODE_SERVICE, DBUS_NAME_FLAG_DO_NOT_QUEUE , &error);

  switch( ret )
  {
  case DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER:
    // expected result
    break;

  case DBUS_REQUEST_NAME_REPLY_ALREADY_OWNER:
    // functionally ok, but we do have a logic error somewhere
    log_warning("already owning '%s'", USB_MODE_SERVICE);
    break;

  default:
    // something odd
    log_err("failed claiming dbus name\n");
    if( dbus_error_is_set(&error) )
        log_debug("DBUS ERROR: %s, %s \n", error.name, error.message);
    goto EXIT;
  }

  dbus_connection_name = TRUE;

  /* everything went fine */
  status = TRUE;

EXIT:
  dbus_error_free(&error);
  return status;
}

/**
 * Clean up the dbus connections for the application
 * synchronisation after sync is done
 */
void usb_moded_appsync_cleanup(void)
{
  /* Drop the service name - if we have it */
  if (dbus_connection_ses != NULL )
  {
    if( dbus_connection_name )
    {
      DBusError error = DBUS_ERROR_INIT;
      int ret = dbus_bus_release_name(dbus_connection_ses, USB_MODE_SERVICE, &error);

      switch( ret )
      {
      case DBUS_RELEASE_NAME_REPLY_RELEASED:
	// as expected
	break;
      case DBUS_RELEASE_NAME_REPLY_NON_EXISTENT:
	// weird, but since nobody owns the name ...
	break;
      case DBUS_RELEASE_NAME_REPLY_NOT_OWNER:
	log_warning("somebody else owns '%s'", USB_MODE_SERVICE);
      }

      dbus_connection_name = FALSE;

      if( dbus_error_is_set(&error) )
      {
	log_debug("DBUS ERROR: %s, %s \n", error.name, error.message);
	dbus_error_free(&error);
      }
    }
  }
}

/**
 * Launch applications over dbus that need to be synchronized
 */
int usb_moded_dbus_app_launch(const char *launch)
{
  int ret = -1; // assume failure

  if( dbus_connection_ses == 0 )
  {
    log_err("could not start '%s': no session bus connection", launch);
  }
  else
  {
    DBusError error = DBUS_ERROR_INIT;
    if( !dbus_bus_start_service_by_name(dbus_connection_ses, launch, 0, NULL, &error) )
    {
      log_err("could not start '%s': %s: %s", launch, error.name, error.message);
      dbus_error_free(&error);
    }
    else
    {
      ret = 0; // success
    }
  }
  return ret;
}
