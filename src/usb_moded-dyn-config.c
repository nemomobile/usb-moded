/**
  @file usb_moded-dyn-mode.c
 
  Copyright (C) 2011 Nokia Corporation. All rights reserved.

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

#include "usb_moded-dyn-config.h"
#include "usb_moded-log.h"

static struct mode_list_elem *read_mode_file(const gchar *filename);

GList *read_mode_list(void)
{
  GDir *confdir;
  GList *modelist = NULL;
  const gchar *dirname;
  struct mode_list_elem *list_item;

  confdir = g_dir_open(MODE_DIR_PATH, 0, NULL);
  if(confdir)
  {
    while((dirname = g_dir_read_name(confdir)) != NULL)
	{
		log_debug("Read file %s\n", dirname);
		list_item = read_mode_file(dirname);
		if(list_item)
			modelist = g_list_append(modelist, list_item);
	}
    g_dir_close(confdir);
  }
  else
	  log_debug("Mode confdir open failed or file is incomplete/invalid.\n");
  return(modelist);
}

static struct mode_list_elem *read_mode_file(const gchar *filename)
{
  GKeyFile *settingsfile;
  gboolean test = FALSE;
  struct mode_list_elem *list_item = NULL;
  gchar *full_filename = NULL;

  full_filename = g_strconcat(MODE_DIR_PATH, "/", filename, NULL);

  settingsfile = g_key_file_new();
  test = g_key_file_load_from_file(settingsfile, full_filename, G_KEY_FILE_NONE, NULL);
  /* free full_filename immediately as we do not use it anymore */
  free(full_filename);
  if(!test)
  {
      return(NULL);
  }
  list_item = malloc(sizeof(struct mode_list_elem));
  list_item->mode_name = g_key_file_get_string(settingsfile, MODE_ENTRY, MODE_NAME_KEY, NULL);
  log_debug("Dynamic mode name = %s\n", list_item->mode_name);
  list_item->mode_module = g_key_file_get_string(settingsfile, MODE_ENTRY, MODE_MODULE_KEY, NULL);
  log_debug("Dynamic mode module = %s\n", list_item->mode_module);
  list_item->appsync = g_key_file_get_integer(settingsfile, MODE_ENTRY, MODE_NEEDS_APPSYNC_KEY, NULL);
  list_item->network = g_key_file_get_integer(settingsfile, MODE_ENTRY, MODE_NETWORK_KEY, NULL);
  list_item->network_interface = g_key_file_get_string(settingsfile, MODE_ENTRY, MODE_NETWORK_INTERFACE_KEY, NULL);
  list_item->sysfs_path = g_key_file_get_string(settingsfile, MODE_OPTIONS_ENTRY, MODE_SYSFS_PATH, NULL);
  //log_debug("Dynamic mode sysfs path = %s\n", list_item->sysfs_path);
  list_item->sysfs_value = g_key_file_get_string(settingsfile, MODE_OPTIONS_ENTRY, MODE_SYSFS_VALUE, NULL);
  //log_debug("Dynamic mode sysfs value = %s\n", list_item->sysfs_value);
  list_item->sysfs_reset_value = g_key_file_get_string(settingsfile, MODE_OPTIONS_ENTRY, MODE_SYSFS_RESET_VALUE, NULL);
  list_item->softconnect = g_key_file_get_string(settingsfile, MODE_OPTIONS_ENTRY, MODE_SOFTCONNECT, NULL);
  list_item->softconnect_disconnect = g_key_file_get_string(settingsfile, MODE_OPTIONS_ENTRY, MODE_SOFTCONNECT_DISCONNECT, NULL);
  list_item->softconnect_path = g_key_file_get_string(settingsfile, MODE_OPTIONS_ENTRY, MODE_SOFTCONNECT_PATH, NULL);
  list_item->android_extra_sysfs_path = g_key_file_get_string(settingsfile, MODE_OPTIONS_ENTRY, MODE_ANDROID_EXTRA_SYSFS_PATH, NULL);
  list_item->android_extra_sysfs_path2 = g_key_file_get_string(settingsfile, MODE_OPTIONS_ENTRY, MODE_ANDROID_EXTRA_SYSFS_PATH2, NULL);
  //log_debug("Android extra mode sysfs path2 = %s\n", list_item->android_extra_sysfs_path2);
  list_item->android_extra_sysfs_value = g_key_file_get_string(settingsfile, MODE_OPTIONS_ENTRY, MODE_ANDROID_EXTRA_SYSFS_VALUE, NULL);
  list_item->android_extra_sysfs_value2 = g_key_file_get_string(settingsfile, MODE_OPTIONS_ENTRY, MODE_ANDROID_EXTRA_SYSFS_VALUE2, NULL);
  //log_debug("Android extra value2 = %s\n", list_item->android_extra_sysfs_value2);
  list_item->idProduct = g_key_file_get_string(settingsfile, MODE_OPTIONS_ENTRY, MODE_IDPRODUCT, NULL);

  g_key_file_free(settingsfile);
  if(list_item->mode_name == NULL || list_item->mode_module == NULL)
  {
	/* free list_item as it will not be used */
	free(list_item);
	return NULL;
  }
  if(list_item->network && list_item->network_interface == NULL)
  {
	/* free list_item as it will not be used */
	free(list_item);
	return NULL;
  }
  if(list_item->sysfs_path && list_item->sysfs_value == NULL)
  {
	/* free list_item as it will not be used */
	free(list_item);
	return NULL;
  }
  if(list_item->softconnect &&  list_item->softconnect_path == NULL)
  {
	/* free list_item as it will not be used */
	free(list_item);
	return NULL;
  }
  else
  	return(list_item);
}

