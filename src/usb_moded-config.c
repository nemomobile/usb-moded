/**
  @file usb_moded-config.c
 
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
#include <glib/gkeyfile.h>

#include "usb_moded-config.h"
#include "usb_moded-log.h"

const char *find_mounts(void)
{
  GKeyFile *settingsfile;
  gboolean test = FALSE;
  gchar **keys;
  const char *ret = NULL, *tmp_char;

  settingsfile = g_key_file_new();
  test = g_key_file_load_from_file(settingsfile, FS_MOUNT_CONFIG_FILE, G_KEY_FILE_NONE, NULL);
  if(!test)
  {
      ret = g_strdup(FS_MOUNT_DEFAULT);	  
      log_debug("Module parameter string = %s\n", ret);
      g_key_file_free(settingsfile);
      return(ret);
  }
  keys = g_key_file_get_keys (settingsfile, FS_MOUNT_ENTRY, NULL, NULL);
  while (*keys != NULL)
  {
  	if(!strcmp(*keys, FS_MOUNT_KEY))
	{
		tmp_char = g_key_file_get_string(settingsfile, FS_MOUNT_ENTRY, *keys, NULL);
		if(tmp_char)
		{
			ret = g_strdup(tmp_char);
  			log_debug("Module parameter string = %s\n", ret);
		}
	}
	keys++;
  }
  g_key_file_free(settingsfile);
  return(ret);
}

int find_sync(void)
{
  GKeyFile *settingsfile;
  gboolean test = FALSE;
  gchar **keys;
  int ret = 0;

  settingsfile = g_key_file_new();
  test = g_key_file_load_from_file(settingsfile, FS_MOUNT_CONFIG_FILE, G_KEY_FILE_NONE, NULL);
  if(!test)
  {
      log_debug("Sync is default  = %d\n", ret);
      g_key_file_free(settingsfile);
      return(ret);
  }
  keys = g_key_file_get_keys (settingsfile, FS_SYNC_ENTRY, NULL, NULL);
  while (*keys != NULL)
  {
  	if(!strcmp(*keys, FS_SYNC_KEY))
	{
		ret = g_key_file_get_integer(settingsfile, FS_SYNC_ENTRY, *keys, NULL);
		log_debug("key value  = %d\n", ret);
	}
	keys++;
  }
  g_key_file_free(settingsfile);
  return(ret);
}

const char * find_alt_mount(void)
{
  GKeyFile *settingsfile;
  gboolean test = FALSE;
  gchar **keys;
  const char *ret = NULL, *tmp_char;

  settingsfile = g_key_file_new();
  test = g_key_file_load_from_file(settingsfile, FS_MOUNT_CONFIG_FILE, G_KEY_FILE_NONE, NULL);
  if(!test)
  {
      log_debug("Keyfile unavailable no fallback mount.");
      g_key_file_free(settingsfile);
      return(ret);
  }
  keys = g_key_file_get_keys (settingsfile, ALT_MOUNT_ENTRY, NULL, NULL);
  while (*keys != NULL)
  {
        if(!strcmp(*keys, ALT_MOUNT_KEY))
        {
                tmp_char = g_key_file_get_string(settingsfile, ALT_MOUNT_ENTRY, *keys, NULL);
                if(tmp_char)
		{
			ret = g_strdup(tmp_char);
  			log_debug("Alternate mount = %s\n", ret);
		}
        }
        keys++;
  }
  g_key_file_free(settingsfile);
  return(ret);
}

#ifdef NOKIA
const char * find_cdrom_path(void)
{
  GKeyFile *settingsfile;
  gboolean test = FALSE;
  gchar **keys;
  const char *ret = NULL, *tmp_char;

  settingsfile = g_key_file_new();
  test = g_key_file_load_from_file(settingsfile, FS_MOUNT_CONFIG_FILE, G_KEY_FILE_NONE, NULL);
  if(!test)
  {
      log_debug("No cdrom path.\n");
      g_key_file_free(settingsfile);
      return(ret);
  }
  keys = g_key_file_get_keys (settingsfile, CDROM_PATH_ENTRY, NULL, NULL);
  while (*keys != NULL)
  {
  	if(!strcmp(*keys, CDROM_PATH_KEY))
	{
		tmp_char = g_key_file_get_string(settingsfile, CDROM_PATH_ENTRY, *keys, NULL);
		if(tmp_char)
		{
			log_debug("cdrom key value  = %s\n", tmp_char);
			ret = g_strdup(tmp_char);
		}
	}
	keys++;
  }
  g_key_file_free(settingsfile);
  return(ret);
}
#endif /* NOKIA */
