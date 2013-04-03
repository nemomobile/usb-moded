/*
  Copyright (C) 2010 Nokia Corporation. All rights reserved.

  Author: Philippe De Swert <philippe.de-swert@nokia.com>

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

#ifndef ANDROID
/* module name definitions */
#define MODULE_NETWORK          "g_nokia" 
#ifdef NOKIA
#define MODULE_NETWORK_MTP      "g_nokia devmode=0" 
#else
#define MODULE_NETWORK_MTP      "g_nokia" 
#endif /* NOKIA */
#define MODULE_MASS_STORAGE     "g_mass_storage"
#define MODULE_FILE_STORAGE	"g_file_storage"
#define MODULE_CHARGING		"g_mass_storage luns=1 stall=0 removable=1"
#define MODULE_CHARGE_FALLBACK	"g_file_storage luns=1 stall=0 removable=1"
#define MODULE_NONE             "none"
#define MODULE_DEVELOPER	"g_ether"
#define MODULE_MTP		"g_ffs"
#else
#define MODULE_MASS_STORAGE     "mass_storage"
#define MODULE_FILE_STORAGE	"mass_storage"
#define MODULE_DEVELOPER	"rndis"
#define MODULE_MTP		"mtp"
#define MODULE_NONE             "none"
#define MODULE_CHARGING		"mass_storage"
#endif

/* load module */
int usb_moded_load_module(const char *module);

/* unload module */
int usb_moded_unload_module(const char *module);

/* find which module is loaded */
const char * usb_moded_find_module(void);

/* clean up modules when usb gets disconnected */
int usb_moded_module_cleanup(const char *module);

/* clean up to allow for switching */
int usb_moded_module_switch_prepare(int force);

/* check if the correct module is loaded or not  and switch if not */
void check_module_state(const char *module_name);

#ifdef NOKIA
/* handler for a cleanup after a timeout */
gboolean usb_cleanup_timeout(gpointer data);
#endif /* NOKIA */
