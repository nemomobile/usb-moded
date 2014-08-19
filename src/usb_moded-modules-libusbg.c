/**
  @file usb_moded-modules-libusbg.c
 
  Copyright (C) 2014 Jolla. All rights reserved.

  @author: Philippe De Swert <philippe.deswert@jolla.com>

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

#include <usbg/usbg.h>

#include "usb_moded.h"
#include "usb_moded-modules.h"
#include "usb_moded-log.h"
#include "usb_moded-config.h"
#include "usb_moded-dbus.h"
#include "usb_moded-dbus-private.h"
#include "usb_moded-config.h"
#include "usb_moded-modesetting.h"

/* module context init and cleanup functions */
void usb_moded_module_ctx_init(void)
{
        usbg_state *s;
        int usbg_ret;

        usbg_ret = usbg_init("/sys/kernel/config", &s);
        if (usbg_ret != USBG_SUCCESS)
                fprintf(stderr, "Error on USB gadget init\n");

}

void usb_moded_module_ctx_cleanup(void)
{
}

/** load module 
 *
 * @param module Name of the module to load
 * or in this case tree to set up and activate
 * @return 0 on success, non-zero on failure
 *
 */
int usb_moded_load_module(const char *module)
{
  return (0);
}

/** unload module
 *  
 * @param module Name of the module to unload
 * or in this case deactivate the gadget
 * @return 0 on success, non-zero on failure
 *
 */
int usb_moded_unload_module(const char *module)
{
  return (0);
}

/** Check which state a module is in
 * or in this case check what state the gadgetfs tree is
 *
 * @return 1 if loaded, 0 when not loaded
 */
inline static int module_state_check(const char *module)
{
	return(0);
}

/** find which module is loaded 
 *
 * @return The name of the loaded module, or NULL if no modules are loaded.
 */
inline const char * usb_moded_find_module(void)
{
  return(0);
}

/** clean up for modules when usb gets disconnected
 *
 * @param module The name of the module to unload
 * @return 0 on success, non-zero on failure
 *
 */
inline int usb_moded_module_cleanup(const char *module)
{
	return(0);
}

/** try to unload modules to support switching
 *
 *
 * @param force force unloading with a nasty clean-up on TRUE, or just try unloading when FALSE
 * @return 0 on success, 1 on failure, 2 if hard clean-up failed
 */
inline int usb_moded_module_switch_prepare (int force)
{
	return 0;
}

/** check for loaded modules and clean-up if they are not for the chosen mode 
 *
 * @param module_name  module name to check for
 *
 */
inline void check_module_state(const char *module_name)
{
  return;
}


