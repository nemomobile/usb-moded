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

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include <glib.h>
#include <glib/gstdio.h>

#include "usb_moded-appsync.h"
#include "usb_moded-appsync-dbus.h"
#include "usb_moded-appsync-dbus-private.h"
#include "usb_moded-log.h"

static struct list_elem *read_file(const gchar *filename);

GList *readlist(void)
{
  GDir *confdir;
  GList *applist = NULL;
  const gchar *dirname;
  struct list_elem *list_item;

  confdir = g_dir_open(CONF_DIR_PATH, 0, NULL);
  if(confdir)
  {
    while((dirname = g_dir_read_name(confdir)) != NULL)
	{
		log_debug("Read file %s\n", dirname);
		list_item = read_file(dirname);
		if(list_item)
			applist = g_list_append(applist, list_item);
	}
    g_dir_close(confdir);
  }
  else
	  log_debug("confdir open failed.\n");
  return(applist);
}

static struct list_elem *read_file(const gchar *filename)
{
  GKeyFile *settingsfile;
  gboolean test = FALSE;
  gchar **keys;
  struct list_elem *list_item = NULL;
  gchar *full_filename = NULL;

  full_filename = g_strconcat(CONF_DIR_PATH, "/", filename, NULL);

  settingsfile = g_key_file_new();
  test = g_key_file_load_from_file(settingsfile, full_filename, G_KEY_FILE_NONE, NULL);
  /* free full_filename immediately as we do not use it anymore */
  free(full_filename);
  if(!test)
  {
      return(NULL);
  }
  list_item = malloc(sizeof(struct list_elem));
  keys = g_key_file_get_keys (settingsfile, APP_INFO_ENTRY , NULL, NULL);
  while (*keys != NULL)
  {
  	if(!strcmp(*keys, APP_INFO_NAME_KEY))
	{
		list_item->name = g_key_file_get_string(settingsfile, APP_INFO_ENTRY, *keys, NULL);
  		log_debug("Appname = %s\n", list_item->name);
	}
  	else if(!strcmp(*keys, APP_INFO_LAUNCH_KEY))
	{
		list_item->launch = g_key_file_get_string(settingsfile, APP_INFO_ENTRY, *keys, NULL);
  		log_debug("launch path = %s\n", list_item->launch);
	}
	keys++;
  }
  g_strfreev(keys);
  g_key_file_free(settingsfile);
  if(list_item->launch == NULL || list_item->name == NULL)
  {
	/* free list_item as it will not be used */
	free(list_item);
	return NULL;
  }
  else
  	return(list_item);
}

int activate_sync(GList *list)
{
  GList *list_iter;

 list_iter = list;
 /* set list to inactive */
  do
    {
	struct list_elem *data = list_iter->data;
	data->active = 0;
	list_iter = g_list_next(list_iter);
    }
  while(list_iter != NULL);

  /* add dbus filter. Use session bus for ready method call? */
  if(!usb_moded_app_sync_init(list))
    {
      log_debug("dbus setup failed => activate immediately \n");
      enumerate_usb(NULL);
      return(1);
   }

  /* go through list and launch apps */
 list_iter = list;
  do
    {
      struct list_elem *data = list_iter->data;
      log_debug("launching app %s\n", data->launch);
      usb_moded_dbus_app_launch(data->launch);
      list_iter = g_list_next(list_iter);
    }
  while(list_iter != NULL);

  /* start timer */
  log_debug("Starting timer\n");
  g_timeout_add_seconds(2, enumerate_usb, list);

  return(0);
}

int mark_active(GList *list, const gchar *name)
{
  int ret = 0;
  static int list_length=0;
  int counter=0;
  GList *list_iter;

  log_debug("app %s notified it is ready\n", name);
  if(list_length == 0)
	  list_length = g_list_length(list);
  
  list_iter = list;
  do
    {
      struct list_elem *data = list_iter->data;
      if(!strcmp(data->name, name))
      {
	if(!data->active)
	{
		data->active = 1;
		counter++;
		
  		if(list_length == counter)
		{
			counter = 0;
			log_debug("All apps active. Let's enumerate\n");
			enumerate_usb(list);
		}
  		ret = 0;
		break;
	}
	else	
	{
		ret = 1;
	}
      }
      list_iter = g_list_next(list_iter);
    }
  while(list_iter != NULL);

  return(ret); 
}

gboolean enumerate_usb(gpointer data)
{
  
  /* activate usb connection/enumeration */
  system("echo 1 > /sys/devices/platform/musb_hdrc/gadget/softconnect");
  log_debug("Softconnect enumeration done\n");

  /* no need to remove timer */

  /* remove dbus filter */
  if(data != NULL)
	  usb_moded_appsync_cleanup((GList *)data);

  /* return false to stop the timer from repeating */
  return(FALSE);
}
