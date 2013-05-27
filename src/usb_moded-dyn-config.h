/*
 
  Copyright (C) 2011 Nokia Corporation. All rights reserved.

  author: Philippe De Swert <philippe.de-swert@nokia.com>

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

#ifndef USB_MODED_DYN_CONFIG_H_
#define USB_MODED_DYN_CONFIG_H_


#define MODE_DIR_PATH	"/etc/usb-moded/dyn-modes"

#define MODE_ENTRY			"mode"
#define MODE_NAME_KEY			"name"
#define MODE_MODULE_KEY			"module"
#define MODE_NEEDS_APPSYNC_KEY		"appsync"
#define MODE_NETWORK_KEY		"network"
#define MODE_NETWORK_INTERFACE_KEY	"network_interface"
#define MODE_OPTIONS_ENTRY		"options"
#define MODE_SYSFS_PATH			"sysfs_path"
#define MODE_SYSFS_VALUE		"sysfs_value"
#define MODE_SOFTCONNECT		"softconnect"
#define MODE_SOFTCONNECT_PATH		"softconnect_path"

/**
 * Struct keeping all the data needed for the definition of a dynamic mode
 */
typedef struct mode_list_elem
{
  /*@{ */
  char *mode_name;		/* mode name */
  char *mode_module;		/* needed module for given mode */
  int appsync;			/* requires appsync or not */
  int network;			/* bring up network or not */
  char *network_interface;	/* Which network interface to bring up if network needs to be enabled */
  char *sysfs_path;		/* path to set sysfs options */
  char *sysfs_value;		/* option name/value to write to sysfs */
  char *softconnect;		/* value to be written to softconnect interface */
  char *softconnect_path;	/* path for the softconnect */
  /*@} */
}mode_list_elem;


GList *read_mode_list(void);

#endif /* USB_MODED_DYN_CONFIG_H_ */
