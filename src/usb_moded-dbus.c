/**
  @file	usb_moded-dbus.c

  Copyright (C) 2010 Nokia Corporation. All rights reserved.
  Copyright (C) 2012 Jolla. All rights reserved.

  @author: Philippe De Swert <philippe.de-swert@nokia.com>
  @author: Philippe De Swert <philippe.deswert@jollamobile.com>

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
#include "usb_moded-modes.h"
#include "usb_moded-config-private.h"
#include "usb_moded-log.h"

#ifdef NOKIA
static DBusHandlerResult usb_moded_dsme_cb(DBusConnection *conn, DBusMessage *msg, void *user_data);
#endif

static DBusConnection *dbus_connection_sys = NULL;
extern gboolean rescue_mode;

static DBusHandlerResult msg_handler(DBusConnection *const connection, DBusMessage *const msg, gpointer const user_data)
{
  DBusHandlerResult   status    = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  DBusMessage        *reply     = 0;
  const char         *interface = dbus_message_get_interface(msg);
  const char         *member    = dbus_message_get_member(msg);
  const char         *object    = dbus_message_get_path(msg);
  int                 type      = dbus_message_get_type(msg);
  const char *xml = 	"<!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\" "
    		    	"\"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">\n"
    		    	"<node name=\"/com/meego/usb_moded\">\n"
    		    	"  <interface name=\"com.meego.usb_moded\">\n"
    		    	"    <method name=\"mode_request\">\n"
    	            	"      <arg name=\"mode\" type=\"s\" direction=\"out\"/>\n"
    	            	"    </method>\n"
    	            	"    <method name=\"set_mode\">\n"
    		    	"      <arg name=\"mode\" type=\"s\" direction=\"in\"/>\n"
    		    	"    </method>\n"
    		    	"    <signal name=\"sig_usb_state_ind\">\n"
    			"      <arg name=\"mode\" type=\"s\"/>\n"
    			"    </signal>\n"
    			"  </interface>\n"
    			"	</node>\n";

 
  (void)user_data;

  if(!interface || !member || !object) goto EXIT;

  if( type == DBUS_MESSAGE_TYPE_METHOD_CALL && !strcmp(interface, USB_MODE_INTERFACE) && !strcmp(object, USB_MODE_OBJECT))
  {
  	status = DBUS_HANDLER_RESULT_HANDLED;
  	
    	if(!strcmp(member, USB_MODE_STATE_REQUEST))
    	{
      		const char *mode = get_usb_mode();

      		if((reply = dbus_message_new_method_return(msg)))
        		dbus_message_append_args (reply, DBUS_TYPE_STRING, &mode, DBUS_TYPE_INVALID);
    	}
    	else if(!strcmp(member, USB_MODE_STATE_SET))
    	{
      		char *use = 0;
      		DBusError   err = DBUS_ERROR_INIT;

      		if(!dbus_message_get_args(msg, &err, DBUS_TYPE_STRING, &use, DBUS_TYPE_INVALID))
        		reply = dbus_message_new_error(msg, DBUS_ERROR_INVALID_ARGS, member);
		else
		{
				/* check if usb is connected, since it makes no sense to change mode if it isn't */
				if(!get_usb_connection_state())
				{
					log_warning("USB not connected, not changing mode!\n");
					goto error_reply;
				}
				/* check if the mode exists */
				if(valid_mode(use))
					goto error_reply;
				/* do not change mode if the mode requested is the one already set */
				if(strcmp(use, get_usb_mode()) != 0)
					set_usb_mode(use);
      				if((reply = dbus_message_new_method_return(msg)))
			        	dbus_message_append_args (reply, DBUS_TYPE_STRING, &use, DBUS_TYPE_INVALID);
				else
error_reply:
       					reply = dbus_message_new_error(msg, DBUS_ERROR_INVALID_ARGS, member);
		}
		dbus_error_free(&err);
        }
	else if(!strcmp(member, USB_MODE_CONFIG_SET))
	{
	  	char *config = 0;
      		DBusError   err = DBUS_ERROR_INIT;

      		if(!dbus_message_get_args(msg, &err, DBUS_TYPE_STRING, &config, DBUS_TYPE_INVALID))
        		reply = dbus_message_new_error(msg, DBUS_ERROR_INVALID_ARGS, member);
		else
		{
			/* error checking is done when setting the GConf key */
			if(!set_mode_setting(config))
			{
 				if((reply = dbus_message_new_method_return(msg)))
			       	dbus_message_append_args (reply, DBUS_TYPE_STRING, &config, DBUS_TYPE_INVALID);
			}
			else
       				reply = dbus_message_new_error(msg, DBUS_ERROR_INVALID_ARGS, config);
		}
		dbus_error_free(&err);	
	}
	else if(!strcmp(member, USB_MODE_NETWORK_SET))
	{
	  	char *config = 0, *setting = 0;
      		DBusError   err = DBUS_ERROR_INIT;

      		if(!dbus_message_get_args(msg, &err, DBUS_TYPE_STRING, &config, DBUS_TYPE_STRING, &setting, DBUS_TYPE_INVALID))
        		reply = dbus_message_new_error(msg, DBUS_ERROR_INVALID_ARGS, member);
		else
		{
			/* error checking is done when setting the GConf key */
			if(!set_network_setting(config, setting))
			{
 				if((reply = dbus_message_new_method_return(msg)))
			       	dbus_message_append_args (reply, DBUS_TYPE_STRING, &config, DBUS_TYPE_STRING, &setting, DBUS_TYPE_INVALID);
			}
			else
       				reply = dbus_message_new_error(msg, DBUS_ERROR_INVALID_ARGS, config);
		}
		dbus_error_free(&err);	
	}
	else if(!strcmp(member, USB_MODE_CONFIG_GET))
	{
		const char *config = get_mode_setting();
		
      		if((reply = dbus_message_new_method_return(msg)))
        		dbus_message_append_args (reply, DBUS_TYPE_STRING, &config, DBUS_TYPE_INVALID);
	}
	else if(!strcmp(member, USB_MODE_RESCUE_OFF))
	{
		rescue_mode = FALSE;
		reply = dbus_message_new_method_return(msg);
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
  }
  else if( type == DBUS_MESSAGE_TYPE_METHOD_CALL && !strcmp(interface, "org.freedesktop.DBus.Introspectable") &&
           !strcmp(object, USB_MODE_OBJECT) && !strcmp(member, "Introspect"))
  {
  	status = DBUS_HANDLER_RESULT_HANDLED;

      	if((reply = dbus_message_new_method_return(msg)))
               dbus_message_append_args (reply, DBUS_TYPE_STRING, &xml, DBUS_TYPE_INVALID);
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
 * Init dbus for usb_moded
 *
 * @return TRUE when everything went ok
 */
gboolean usb_moded_dbus_init(void)
{
  gboolean status = FALSE;
  DBusError error;
  int ret;

  dbus_error_init(&error);

  /* connect to system bus */
  if ((dbus_connection_sys = dbus_bus_get(DBUS_BUS_SYSTEM, &error)) == NULL) 
  {
   	log_debug("Failed to open connection to system message bus; %s\n",  error.message);
        goto EXIT;
  }

  /* Initialise message handlers */
  if (!dbus_connection_add_filter(dbus_connection_sys, msg_handler, NULL, NULL)) 
   	goto EXIT;

  /* Acquire D-Bus service */
  ret = dbus_bus_request_name(dbus_connection_sys, USB_MODE_SERVICE, DBUS_NAME_FLAG_DO_NOT_QUEUE, &error);
  if (ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) 
  {
	log_debug("failed claiming dbus name\n");
	if( dbus_error_is_set(&error) )
		log_debug("DBUS ERROR: %s, %s \n", error.name, error.message);
        goto EXIT;
  }

  /* only match on signals/methods we support (if needed)
  dbus_bus_add_match(dbus_connection_sys, USB_MODE_INTERFACE, &error);
  */

  dbus_threads_init_default();

  /* Connect D-Bus to the mainloop */
  dbus_connection_setup_with_g_main(dbus_connection_sys, NULL);

  /* everything went fine */
  status = TRUE;

EXIT:		
  dbus_error_free(&error);
  return status;
}

/**
 * Clean up the dbus connections on exit
 *
 */
void usb_moded_dbus_cleanup(void)
{
  /* clean up system bus connection */
  if (dbus_connection_sys != NULL) 
  {
	  dbus_connection_remove_filter(dbus_connection_sys, msg_handler, NULL);
	  dbus_connection_unref(dbus_connection_sys);
          dbus_connection_sys = NULL;
  }
}

/**
 * Send regular usb_moded state signal
 *
 * @return 1 on success, 0 on failure
 * @param state_ind the signal name
 *
*/
int usb_moded_send_signal(const char *state_ind)
{
  int result = 1;
  DBusMessage* msg = 0;

  // create a signal and check for errors 
  msg = dbus_message_new_signal(USB_MODE_OBJECT, USB_MODE_INTERFACE, USB_MODE_SIGNAL_NAME );
  if (NULL == msg) 
  { 
      log_debug("Message Null\n"); 
      goto EXIT;
   }

  // append arguments onto signal
  if (!dbus_message_append_args(msg, DBUS_TYPE_STRING, &state_ind, DBUS_TYPE_INVALID)) 
  { 
      log_debug("Appending arguments failed. Out Of Memory!\n"); 
      goto EXIT;
  }

  // send the message on the correct bus  and flush the connection
  if (!dbus_connection_send(dbus_connection_sys, msg, 0)) 
  {
 	log_debug("Failed sending message. Out Of Memory!\n");
	goto EXIT;
  }
  result = 0;

EXIT:   
  // free the message 
  if(msg != 0)
	  dbus_message_unref(msg);

  return result;
}

int usb_moded_send_error_signal(const char *error)
{
  int result = 1;
  DBusMessage* msg = 0;

  // create a signal and check for errors 
  msg = dbus_message_new_signal(USB_MODE_OBJECT, USB_MODE_INTERFACE, USB_MODE_ERROR_SIGNAL_NAME );
  if (NULL == msg) 
  { 
      log_debug("Message Null\n"); 
      goto EXIT;
   }

  // append arguments onto signal
  if (!dbus_message_append_args(msg, DBUS_TYPE_STRING, &error, DBUS_TYPE_INVALID)) 
  { 
      log_debug("Appending arguments failed. Out Of Memory!\n"); 
      goto EXIT;
  }

  // send the message on the correct bus  and flush the connection
  if (!dbus_connection_send(dbus_connection_sys, msg, 0)) 
  {
 	log_debug("Failed sending message. Out Of Memory!\n");
	goto EXIT;
  }
  result = 0;

EXIT:   
  // free the message 
  if(msg != 0)
	  dbus_message_unref(msg);

  return result;
}

#ifdef NOKIA
int usb_moded_dsme_listener(void)
{
  DBusError       err = DBUS_ERROR_INIT;
  DBusConnection *dbus_conn_dsme = NULL;

  if( (dbus_conn_dsme = dbus_bus_get(DBUS_BUS_SYSTEM, &err)) == 0 )
  {
         log_err("Could not connect to dbus for dsme\n");
         goto cleanup;
  }

  dbus_bus_add_match(dbus_conn_dsme, "type='signal',interface='com.nokia.dsme.signal',path='/com/nokia/dsme/signal'", &err);
  if( dbus_error_is_set(&err) )
  {
    goto cleanup;
  }
  if( !dbus_connection_add_filter(dbus_conn_dsme, usb_moded_dsme_cb , 0, 0) )
  {
        log_err("adding system dbus filter for dsme failed");
    goto cleanup;
  }
  dbus_connection_setup_with_g_main(dbus_conn_dsme, NULL);

cleanup:
  dbus_error_free(&err);
  return(1);
}

static DBusHandlerResult usb_moded_dsme_cb(DBusConnection *conn, DBusMessage *msg, void *user_data)
{
  DBusHandlerResult   result    = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  const char         *interface = dbus_message_get_interface(msg);
  const char         *member    = dbus_message_get_member(msg);
  const char         *object    = dbus_message_get_path(msg);
  int                 type      = dbus_message_get_type(msg);

  (void) user_data;

  // sanity checks
  if( !interface || !member || !object ) goto cleanup;
  if( type != DBUS_MESSAGE_TYPE_SIGNAL ) goto cleanup;
  if( strcmp(interface, "com.nokia.dsme.signal") ) goto cleanup;
  if( strcmp(object, "/com/nokia/dsme/signal") )  goto cleanup;

  // handle known signals
  else if( !strcmp(member, "shutdown_ind") )
  {
        log_debug("Shutdown indication. Cleaning up.\n");
	set_usb_mode(MODE_CHARGING);
  }
  result = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

cleanup:
  return result;
}

#endif /* NOKIA */
