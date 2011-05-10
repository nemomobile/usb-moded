/**
  @file usb_moded-appsync.c

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

#define _GNU_SOURCE

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>

#include <glib.h>
#include <glib/gstdio.h>

#include "usb_moded-appsync.h"
#include "usb_moded-appsync-dbus.h"
#include "usb_moded-appsync-dbus-private.h"
#include "usb_moded-modesetting.h"
#include "usb_moded-log.h"

static struct list_elem *read_file(const gchar *filename);
static gboolean enumerate_usb(gpointer data);

static GList *sync_list = NULL;

static unsigned sync_tag = 0;
static unsigned enum_tag = 0;
static struct timeval sync_tv;

static void free_elem(gpointer aptr)
{
  struct list_elem *elem = aptr;
  free(elem->name);
  free(elem->launch);
  free(elem);
}

static void free_list(void)
{
  if( sync_list != 0 )
  {
    g_list_free_full(sync_list, free_elem);
    sync_list = 0;
  }
}

void readlist(void)
{
  GDir *confdir = 0;

  const gchar *dirname;
  struct list_elem *list_item;

  free_list();

  if( !(confdir = g_dir_open(CONF_DIR_PATH, 0, NULL)) )
    goto cleanup;

  while( (dirname = g_dir_read_name(confdir)) )
  {
    log_debug("Read file %s\n", dirname);
    if( (list_item = read_file(dirname)) )
      sync_list = g_list_append(sync_list, list_item);
  }

cleanup:
  if( confdir ) g_dir_close(confdir);

  /* set up session bus connection if app sync in use
   * so we do not need to make the time consuming connect
   * operation at enumeration time ... */

  if( sync_list )
  {
    usb_moded_app_sync_init_connection();
  }
}

static struct list_elem *read_file(const gchar *filename)
{
  gchar *full_filename = NULL;
  GKeyFile *settingsfile = NULL;
  struct list_elem *list_item = NULL;

  if( !(full_filename = g_strconcat(CONF_DIR_PATH, "/", filename, NULL)) )
    goto cleanup;

  if( !(settingsfile = g_key_file_new()) )
    goto cleanup;

  if( !g_key_file_load_from_file(settingsfile, full_filename, G_KEY_FILE_NONE, NULL) )
    goto cleanup;

  if( !(list_item = calloc(1, sizeof *list_item)) )
    goto cleanup;

  list_item->name = g_key_file_get_string(settingsfile, APP_INFO_ENTRY, APP_INFO_NAME_KEY, NULL);
  log_debug("Appname = %s\n", list_item->name);
  list_item->launch = g_key_file_get_string(settingsfile, APP_INFO_ENTRY, APP_INFO_LAUNCH_KEY, NULL);
  log_debug("Launch = %s\n", list_item->launch);
  list_item->mode = g_key_file_get_string(settingsfile, APP_INFO_ENTRY, APP_INFO_MODE_KEY, NULL);
  log_debug("Launch mode = %s\n", list_item->mode);

cleanup:

  if(settingsfile) 
	g_key_file_free(settingsfile);
  g_free(full_filename);

  /* if not all the elements are filled in we discard the list_item */
  if( list_item && !(list_item->launch && list_item->name && list_item->mode) )
  {
    free_elem(list_item), list_item = 0;
  }

  return list_item;
}

int activate_sync(const char *mode)
{
  GList *iter;

  log_debug("activate sync");

  /* Bump tag, see enumerate_usb() */
  ++sync_tag; gettimeofday(&sync_tv, 0);

  if( sync_list == 0 )
  {
    enumerate_usb(NULL);
    return 0;
  }

  /* set list to inactive, mark other modes as active already */
  for( iter = sync_list; iter; iter = g_list_next(iter) )
  {
    struct list_elem *data = iter->data;
    if(!strcmp(data->mode, mode))
    	data->active = 0;
    else
	data->active = 1;
  }

  /* add dbus filter. Use session bus for ready method call? */
  if(!usb_moded_app_sync_init())
  {
      log_debug("dbus setup failed => activate immediately \n");
      enumerate_usb(NULL);
      return(1);
   }

  /* start timer */
  log_debug("Starting timer\n");
  g_timeout_add_seconds(2, enumerate_usb, NULL);

  /* go through list and launch apps */
  for( iter = sync_list; iter; iter = g_list_next(iter) )
  {
    struct list_elem *data = iter->data;
    if(!strcmp(mode, data->mode))
    {
      log_debug("launching app %s\n", data->launch);
      usb_moded_dbus_app_launch(data->launch);
    }
  }

  return(0);
}

int mark_active(const gchar *name)
{
  int ret = -1; // assume name not found
  int missing = 0;

  GList *iter;

  log_debug("App %s notified it is ready\n", name);

  for( iter = sync_list; iter; iter = g_list_next(iter) )
  {
    struct list_elem *data = iter->data;
    if(!strcmp(data->name, name))
    {
      /* TODO: do we need to worry about duplicate names in the list? */
      ret = !data->active;
      data->active = 1;

      /* updated + missing -> not going to enumerate */
      if( missing ) break;
    }
    else if( data->active == 0 )
    {
      missing = 1;

      /* updated + missing -> not going to enumerate */
      if( ret != -1 ) break;
    }
  }
  if( !missing )
  {
    log_debug("All apps active. Let's enumerate\n");
    enumerate_usb(NULL);
  }
  
  /* -1=not found, 0=already active, 1=activated now */
  return ret; 
}

static gboolean enumerate_usb(gpointer data)
{
  struct timeval tv;

  /* We arrive here twice: when app sync is done
   * and when the app sync timeout gets triggered.
   * The tags are used to filter out these repeats.
   */

  if( enum_tag == sync_tag )
  {
    log_debug("ignoring enumeration trigger");
  }
  else
  {
    /* activate usb connection/enumeration */
    write_to_file("/sys/devices/platform/musb_hdrc/gadget/softconnect", "1");
    log_debug("Softconnect enumeration done\n");

    enum_tag = sync_tag;

    /* Debug: how long it took from sync start to get here */
    gettimeofday(&tv, 0);
    timersub(&tv, &sync_tv, &tv);
    log_debug("sync to enum: %.3f seconds", tv.tv_sec + tv.tv_usec * 1e-6);

    /* remove dbus service */
    usb_moded_appsync_cleanup();
  }
  /* return false to stop the timer from repeating */
  return FALSE;
}
