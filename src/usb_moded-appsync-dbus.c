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

static DBusConnection *dbus_connection_ses = NULL;

static DBusHandlerResult msg_handler(DBusConnection *const connection, DBusMessage *const msg, gpointer const user_data)
{
  DBusHandlerResult   status    = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  DBusMessage        *reply     = 0;
  const char         *interface = dbus_message_get_interface(msg);
  const char         *member    = dbus_message_get_member(msg);
  const char         *object    = dbus_message_get_path(msg);
  int                 type      = dbus_message_get_type(msg);

 
  if(!interface || !member || !object) goto EXIT;

  if( type == DBUS_MESSAGE_TYPE_METHOD_CALL && !strcmp(interface, USB_MODE_INTERFACE) && !strcmp(object, USB_MODE_OBJECT))
  {
  	status = DBUS_HANDLER_RESULT_HANDLED;
  	
    	if(!strcmp(member, USB_MODE_APP_STATE))
    	{
      		char *use = 0;
      		DBusError   err = DBUS_ERROR_INIT;

      		if(!dbus_message_get_args(msg, &err, DBUS_TYPE_STRING, &use, DBUS_TYPE_INVALID))
        		reply = dbus_message_new_error(msg, DBUS_ERROR_INVALID_ARGS, member);
		else
		{

			if(mark_active((GList *)user_data, use))
				goto error_reply;
			else
			{
				if((reply = dbus_message_new_method_return(msg)))
				{
		        		dbus_message_append_args (reply, DBUS_TYPE_STRING, &use, DBUS_TYPE_INVALID);
				}
				else
error_reply:
       				reply = dbus_message_new_error(msg, DBUS_ERROR_INVALID_ARGS, member);
			}
			dbus_error_free(&err);
        	}
  	}
  }
  else
  { 
       	/*unknown methods are handled here */
  	reply = dbus_message_new_error(msg, DBUS_ERROR_UNKNOWN_METHOD, member);
  }

  if( !reply )
  {
  	reply = dbus_message_new_error(msg, DBUS_ERROR_FAILED, member);
  }


EXIT:

  if(reply)
  {
  	if( !dbus_message_get_no_reply(msg) )
    	{	
      		if( !dbus_connection_send(connection, reply, 0) )
        		log_debug("Failed sending reply. Out Of Memory!\n");
      	}
    	dbus_message_unref(reply);
  }

  return status;
}

/**
 * Init dbus for usb_moded application synchronisation
 *
 * @return TRUE when everything went ok
 */
gboolean usb_moded_app_sync_init(GList *list)
{
  gboolean status = FALSE;
  DBusError error;
  int ret;

  dbus_error_init(&error);

  /* connect to session bus */
  if ((dbus_connection_ses = dbus_bus_get_private(DBUS_BUS_SESSION, &error)) == NULL) 
  {
   	log_err("Failed to open connection to session message bus; %s\n",  error.message);
        goto EXIT;
  }

  /* make sure we do not get forced to exit if dbus session dies or stops */
  dbus_connection_set_exit_on_disconnect(dbus_connection_ses, FALSE);

  /* Initialise message handlers */
  if (!dbus_connection_add_filter(dbus_connection_ses, msg_handler, list, NULL)) 
  {
	log_err("failed to add filter\n");
   	goto EXIT;
  }
  /* Acquire D-Bus service */
  ret = dbus_bus_request_name(dbus_connection_ses, USB_MODE_SERVICE, DBUS_NAME_FLAG_REPLACE_EXISTING | DBUS_NAME_FLAG_ALLOW_REPLACEMENT , &error); 
  if (ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) 
  {
	log_err("failed claiming dbus name\n");
	if( dbus_error_is_set(&error) )
		log_debug("DBUS ERROR: %s, %s \n", error.name, error.message);
        goto EXIT; 
  }

  /* Connect D-Bus to the mainloop */
  dbus_connection_setup_with_g_main(dbus_connection_ses, NULL);

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
void usb_moded_appsync_cleanup(GList *list)
{
  DBusError error;

  dbus_error_init(&error);
  /* clean up system bus connection */
  if (dbus_connection_ses != NULL) 
  {
	  dbus_bus_release_name(dbus_connection_ses, USB_MODE_SERVICE, &error);
	  dbus_connection_remove_filter(dbus_connection_ses, msg_handler, list);
	  dbus_connection_close(dbus_connection_ses);
	  dbus_connection_unref(dbus_connection_ses);
          dbus_connection_ses = NULL;
	  log_debug("succesfully cleaned up appsync dbus\n");
  }
  dbus_error_free(&error);
}

/**
 * Launch applications over dbus that need to be synchronized
 */
int usb_moded_dbus_app_launch(const char *launch)
{
  DBusConnection *dbus_conn = NULL;
  DBusError error;
  int ret = 0;

  dbus_error_init(&error);

  if( (dbus_conn = dbus_bus_get(DBUS_BUS_SESSION, &error)) == 0 )
  {
         log_err("Could not connect to dbus session\n");
  }

  dbus_bus_start_service_by_name(dbus_conn, launch, 0, NULL, &error);

  dbus_connection_unref(dbus_conn);

  return(ret);
}
