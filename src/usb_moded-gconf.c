/**
  @file usb_moded-gconf.c

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
/*
 * Gets information from Gconf for the usb modes
*/

/*============================================================================= */

#include <string.h>
#include <stdio.h>

#include <glib.h>
#include <glib-object.h>

#include <gconf/gconf-client.h>

#include "usb_moded-gconf.h"
#include "usb_moded-gconf-private.h"
#include "usb_moded-modes.h"
#include "usb_moded-log.h"
#include "usb_moded.h"

/** Get the config option set in gconf for the default action
 *
 * @return The mode to set by default, or NULL on failure
 *
 */
const char * get_mode_setting(void)
{
  GConfClient *gclient = NULL;
  gchar *mode_value;

  gclient = gconf_client_get_default();

  if (gclient == NULL)
  {
     log_err("Unable to get GConfClient");
     return(0);
  }
  mode_value = gconf_client_get_string(gclient, USB_MODE_GCONF, NULL);

  g_object_unref(gclient);
  return(mode_value);
}

/** Set the config option for the default action
 *
 * @param mode The default action to store in gconf
 * @return 0 on on success, 1 on failure
 *
 */
int set_mode_setting(const char *mode)
{
  GConfClient *gclient = NULL;

  if(!valid_mode(mode) || !strcmp(mode, MODE_ASK))
  {
    gclient = gconf_client_get_default();
    	if (gclient == NULL)
  	{
     		log_err("Unable to get GConfClient");
     		return(1);
  	}
    if (!gconf_client_set_string(gclient, USB_MODE_GCONF, mode, NULL))
    {
     	log_err("Unable to set GConf key");
     	return(1);
    }
  }
  else
  	return(1);

  return(0);
}
