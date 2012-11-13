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
/*
#include <glib/gkeyfile.h>
#include <glib/gstdio.h>
*/
#include "usb_moded-config.h"
#include "usb_moded-config-private.h"
#include "usb_moded-log.h"
#include "usb_moded-modes.h"

static int get_conf_int(const gchar *entry, const gchar *key);
static const char * get_conf_string(const gchar *entry, const gchar *key);

const char *find_mounts(void)
{
  
  const char *ret = NULL;

  ret = get_conf_string(FS_MOUNT_ENTRY, FS_MOUNT_KEY);
  if(ret == NULL)
  {
      ret = g_strdup(FS_MOUNT_DEFAULT);	  
      log_debug("Default mount = %s\n", ret);
  }
  return(ret);
}

int find_sync(void)
{

  return(get_conf_int(FS_SYNC_ENTRY, FS_SYNC_KEY));
}

const char * find_alt_mount(void)
{
  return(get_conf_string(ALT_MOUNT_ENTRY, ALT_MOUNT_KEY));
}

#ifdef UDEV
const char * find_udev_path(void)
{
  return(get_conf_string(UDEV_PATH_ENTRY, UDEV_PATH_KEY));
}

const char * find_udev_subsystem(void)
{
  return(get_conf_string(UDEV_PATH_ENTRY, UDEV_SUBSYSTEM_KEY));
}
#endif /* UDEV */

#ifdef NOKIA
const char * find_cdrom_path(void)
{
  return(get_conf_string(CDROM_ENTRY, CDROM_PATH_KEY));
}

int find_cdrom_timeout(void)
{
  return(get_conf_int(CDROM_ENTRY, CDROM_TIMEOUT_KEY));
}
#endif /* NOKIA */

#ifdef UDEV
const char * check_trigger(void)
{
  return(get_conf_string(TRIGGER_ENTRY, TRIGGER_PATH_KEY));
}

const char * get_trigger_subsystem(void)
{
  return(get_conf_string(TRIGGER_ENTRY, TRIGGER_UDEV_SUBSYSTEM));
}

const char * get_trigger_mode(void)
{
  return(get_conf_string(TRIGGER_ENTRY, TRIGGER_MODE_KEY));
}

const char * get_trigger_property(void)
{
  return(get_conf_string(TRIGGER_ENTRY, TRIGGER_PROPERTY_KEY));
}

const char * get_trigger_value(void)
{
  return(get_conf_string(TRIGGER_ENTRY, TRIGGER_PROPERTY_VALUE_KEY));
}
#endif /* UDEV */

const char * get_network_ip(void)
{
  return(get_conf_string(NETWORK_ENTRY, NETWORK_IP_KEY));
}

const char * get_network_interface(void)
{
  return(get_conf_string(NETWORK_ENTRY, NETWORK_INTERFACE_KEY));
}

const char * get_network_gateway(void)
{
  return(get_conf_string(NETWORK_ENTRY, NETWORK_GATEWAY_KEY));
}

/* create basic conffile with sensible defaults */
static void create_conf_file(void)
{
  GKeyFile *settingsfile;
  gchar **keys, *keyfile;

  settingsfile = g_key_file_new();

  g_key_file_set_string(settingsfile, MODE_SETTING_ENTRY, MODE_SETTING_KEY, MODE_DEVELOPER );
  keyfile = g_key_file_to_data (settingsfile, NULL, NULL);
  if(g_file_set_contents(FS_MOUNT_CONFIG_FILE, keyfile, -1, NULL) == 0)
	log_debug("conffile creation failed. Continuing without configuration!\n");
}

static int get_conf_int(const gchar *entry, const gchar *key)
{
  GKeyFile *settingsfile;
  gboolean test = FALSE;
  gchar **keys;
  int ret = 0;

  settingsfile = g_key_file_new();
  test = g_key_file_load_from_file(settingsfile, FS_MOUNT_CONFIG_FILE, G_KEY_FILE_NONE, NULL);
  if(!test)
  {
      log_debug("no conffile, Creating\n");
      create_conf_file();
  }
  keys = g_key_file_get_keys (settingsfile, entry, NULL, NULL);
  if(keys == NULL)
        return ret;
  while (*keys != NULL)
  {
        if(!strcmp(*keys, key))
        {
                ret = g_key_file_get_integer(settingsfile, entry, *keys, NULL);
                log_debug("%s key value  = %d\n", key, ret);
        }
        keys++;
  }
  g_key_file_free(settingsfile);
  return(ret);

}

static const char * get_conf_string(const gchar *entry, const gchar *key)
{
  GKeyFile *settingsfile;
  gboolean test = FALSE;
  gchar **keys, *tmp_char = NULL;
  const char *ret = NULL;
  settingsfile = g_key_file_new();
  test = g_key_file_load_from_file(settingsfile, FS_MOUNT_CONFIG_FILE, G_KEY_FILE_NONE, NULL);
  if(!test)
  {
      log_debug("No conffile. Creating\n");
      create_conf_file();
  }
  keys = g_key_file_get_keys (settingsfile, entry, NULL, NULL);
  if(keys == NULL)
        return ret;
  while (*keys != NULL)
  {
        if(!strcmp(*keys, key))
        {
                tmp_char = g_key_file_get_string(settingsfile, entry, *keys, NULL);
                if(tmp_char)
                {
                        log_debug("key %s value  = %s\n", key, tmp_char);
                }
        }
        keys++;
  }
  g_key_file_free(settingsfile);
  return(g_strdup(tmp_char));

}

#ifndef GCONF
const char * get_mode_setting(void)
{
  return(get_conf_string(MODE_SETTING_ENTRY, MODE_SETTING_KEY));
}

int set_mode_setting(const char *mode)
{
  GKeyFile *settingsfile;
  gboolean test = FALSE;
  int ret = 0; 
  gchar *keyfile;

  settingsfile = g_key_file_new();
  test = g_key_file_load_from_file(settingsfile, FS_MOUNT_CONFIG_FILE, G_KEY_FILE_NONE, NULL);
  if(!test)
  {
      log_debug("No conffile. Creating.\n");
      create_conf_file();
  }

  g_key_file_set_string(settingsfile, MODE_SETTING_ENTRY, MODE_SETTING_KEY, mode);
  keyfile = g_key_file_to_data (settingsfile, NULL, NULL); 
  /* free the settingsfile before writing things out to be sure 
     the contents will be correctly written to file afterwards.
     Just a precaution. */
  g_key_file_free(settingsfile);
  ret = g_file_set_contents(FS_MOUNT_CONFIG_FILE, keyfile, -1, NULL);
  
  /* g_file_set_contents returns 1 on succes, since set_mode_settings returns 0 on succes we return the ! value */
  return(!ret);
}


#endif

