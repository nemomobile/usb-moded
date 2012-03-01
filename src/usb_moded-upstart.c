/**
  @file	usb_moded-upstart.c

  Copyright (C) 2012 Nokia Corporation. All rights reserved.

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

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>

#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include "usb_moded-dbus.h"
#include "usb_moded-dbus-private.h"
#include "usb_moded.h"
#include "usb_moded-log.h"

#define SYSTEM_BUS_ADDRESS "unix:path=/var/run/dbus/system_bus_socket"
#define UPSTART_BUS_ADDRESS "unix:abstract=/com/ubuntu/upstart"
#define UPSTART_SERVICE "com.ubuntu.Upstart"
#define UPSTART_REQUEST_PATH "/com/ubuntu/Upstart"
#define UPSTART_INTERFACE "com.ubuntu.Upstart0_6"
#define UPSTART_JOB_INTERFACE "com.ubuntu.Upstart0_6.Job"

static DBusConnection * get_upstart_dbus_connection(void)
{
  DBusError       error;
  DBusConnection *conn = 0;

  dbus_error_init(&error);

  conn = dbus_connection_open_private("unix:abstract=/com/ubuntu/upstart", &error);
  if (!conn) 
  {
     if (dbus_error_is_set(&error))
            log_err("Cannot connect to upstart: %s", error.message);
     else
            log_err("Cannot connect to upstart");
     return 0;
  }

  return conn;
}

static char *get_upstart_job_by_name(const char *name)
{
  DBusError       error;
  DBusConnection *conn = 0;
  DBusMessage *msg = NULL, *reply = NULL;
  char *ret = 0;

  dbus_error_init(&error);
  conn = get_upstart_dbus_connection();
  if(!conn)
	return NULL;
  if ((msg = dbus_message_new_method_call(UPSTART_SERVICE, UPSTART_REQUEST_PATH, UPSTART_INTERFACE, "GetJobByName")) != NULL)
  {
        dbus_message_append_args (msg, DBUS_TYPE_STRING, &name, DBUS_TYPE_INVALID);
        if ((reply = dbus_connection_send_with_reply_and_block(conn, msg, -1, NULL)) != NULL)
        {
            dbus_message_get_args(reply, NULL, DBUS_TYPE_OBJECT_PATH, &ret, DBUS_TYPE_INVALID);
  	    /* log_debug("got following path from upstart: %s for job name %s\n", ret, name);*/
            dbus_message_unref(reply);
        }
        dbus_message_unref(msg);
  }
  dbus_connection_close(conn);
  
  return ret;
}

int upstart_control_job(const char *name, const char * action)
{
  DBusError       error;
  DBusConnection *conn = 0;
  char *job = 0, *arg = 0;
  DBusMessageIter iter, iter2;
  DBusMessage *msg = NULL, *reply = NULL;
  dbus_bool_t wait = FALSE;

  conn = get_upstart_dbus_connection();
  if(!conn)
	return 1;
  job = get_upstart_job_by_name(name);
  if(job)
     if ((msg = dbus_message_new_method_call(UPSTART_SERVICE, job, UPSTART_JOB_INTERFACE, action)) != NULL)
     {
        dbus_message_iter_init_append(msg, &iter);

	if(!dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "s", &iter2))
	{
		log_err("Could not open dbus container\n");
		goto fail;
	}
	asprintf(&arg, "NAME=%s", name);
	if (!dbus_message_iter_append_basic(&iter2, DBUS_TYPE_STRING, &arg))
	{
		log_err("Could not add string array\n");
		dbus_message_iter_abandon_container(&iter, &iter2);
		goto fail;
	}
	if (!dbus_message_iter_close_container(&iter, &iter2)) 
	{
        	log_err( "Cannot close container");
		goto fail;
	}
	if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_BOOLEAN, &wait)) 
	{
		log_err("Could not set wait state\n");
		goto fail;
	}
	if ((reply = dbus_connection_send_with_reply_and_block(conn, msg, -1, NULL)) != NULL)	
        {
          dbus_message_unref(reply);
        }
        dbus_message_unref(msg);
     }
  else
	log_err("No upstart path for %s\n", name);
  dbus_connection_close(conn);
   
  return 0;

fail:
  dbus_connection_close(conn);
  return 1;
}
