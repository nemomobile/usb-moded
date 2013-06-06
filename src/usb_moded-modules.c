/**
  @file usb_moded-modules.c
 
  Copyright (C) 2010 Nokia Corporation. All rights reserved.
  Copyright (C) 2012 Jolla. All rights reserved.

  @author: Philippe De Swert <philippe.de-swert@nokia.com>
  @author: Philippe De Swert <philippe.deswert@jollamobile.com>

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
#include <glib/gprintf.h>

#include <libkmod.h>

#include "usb_moded.h"
#include "usb_moded-modules.h"
#include "usb_moded-log.h"
#include "usb_moded-config.h"
#include "usb_moded-dbus.h"
#include "usb_moded-dbus-private.h"
#include "usb_moded-config.h"
#include "usb_moded-modesetting.h"
#include "usb_moded-modes.h"

/* kmod context - initialized at start in usb_moded_init */
struct kmod_ctx *ctx;

/** load module 
 *
 * @param module Name of the module to load
 * @return 0 on success, non-zero on failure
 *
 */
int usb_moded_load_module(const char *module)
{
	int ret = 0;
	const char * softconnect;

#ifdef NO_KMOD
	gchar *command; 
	
	if(!strcmp(module, MODULE_NONE))
		return 0;
	
	command = g_strconcat("modprobe ", module, NULL);
	ret = system(command);
	if(!strcmp(module, MODULE_MASS_STORAGE) && (ret != 0))
	{
	  command = g_strconcat("modprobe ", MODULE_FILE_STORAGE, NULL);
	  ret = system(command);
	}
	if(!strcmp(module, MODULE_CHARGING) && (ret != 0))
	{
	  command = g_strconcat("modprobe ", MODULE_CHARGE_FALLBACK, NULL);
	  ret = system(command);
	}
	g_free(command);
#else
	const int probe_flags = KMOD_PROBE_APPLY_BLACKLIST;
	struct kmod_module *mod;
	char *charging_args = NULL;
	char *load = NULL;

	if(!strcmp(module, MODULE_NONE))
		return 0;

	/* copy module to load as it might be modified if we're trying charging mode */
	load = strdup(module);
	if(!strcmp(module, MODULE_CHARGING) || !strcmp(module, MODULE_CHARGE_FALLBACK))
	{
	  /* split the string in module name and argument, they are the same for MODULE_CHARGE_FALLBACK 
	     so no need to handle them separately  */
	  gchar **strings;

	  /* since the mass_storage module is the newer one and we check against it to avoid 
	     loading failures we use it here, as we fall back to g_file_storage if g_mass_storage 
	     fails to load */
	  strings = g_strsplit(MODULE_CHARGE_FALLBACK, " ", 2);
	  log_debug("module args = %s, module = %s\n", strings[1], strings[0]);
          charging_args = strdup(strings[1]);
	  /* load was already assigned. Free it to re-assign */
	  free(load);
	  load = strdup(strings[0]);
	  g_strfreev(strings);
	  
	}
	ret = kmod_module_new_from_name(ctx, load, &mod);
	/* since kmod_module_new_from_name does not check if the module
           exists we test it's path in case we deal with the mass-storage one */
	if(!strcmp(module, MODULE_MASS_STORAGE) && 
	    (kmod_module_get_path(mod) == NULL))
	{
	  log_debug("Fallback on older g_file_storage\n");  
	  ret = kmod_module_new_from_name(ctx, MODULE_FILE_STORAGE, &mod);
	}

	if(!charging_args)
		ret = kmod_module_probe_insert_module(mod, probe_flags, NULL, NULL, NULL, NULL);
	else
	{
		ret = kmod_module_probe_insert_module(mod, probe_flags, charging_args, NULL, NULL, NULL);
		free(charging_args);
	}
	kmod_module_unref(mod);
	free(load);
#endif /* NO_KMOD */

	softconnect = get_soft_connect_path();
	if(softconnect)	
	{
		write_to_file(softconnect, "connect");
		free((void *)softconnect);
	}
	if( ret == 0)
		log_info("Module %s loaded successfully\n", module);
	else
		log_info("Module %s failed to load\n", module);
	return(ret);
}

/** unload module
 *  
 * @param module Name of the module to unload
 * @return 0 on success, non-zero on failure
 *
 */
int usb_moded_unload_module(const char *module)
{
	int ret = 0;
	const char * softconnect;


#ifdef NO_KMOD
	gchar *command;

	if(!strcmp(module, MODULE_NONE))
		return 0;

	command = g_strconcat("rmmod ", module, NULL);
	ret = system(command);
	g_free(command);
#else
	struct kmod_module *mod;

	if(!strcmp(module, MODULE_NONE))
		return 0;

	kmod_module_new_from_name(ctx, module, &mod);
	ret = kmod_module_remove_module(mod, KMOD_REMOVE_NOWAIT);
	kmod_module_unref(mod);

#endif /* NO_KMOD */
	softconnect = get_soft_connect_path();
	if(softconnect)	
	{
		write_to_file(softconnect, "disconnect");
		free((void *)softconnect);
	}

	return(ret);
}

/** Check which state a module is in
 *
 * @return 1 if loaded, 0 when not loaded
 */
static int module_state_check(const char *module)
{
  int ret = 0;
  struct kmod_module *mod;

  kmod_module_new_from_name(ctx, module, &mod);
  ret = kmod_module_get_initstate(mod);
  kmod_module_unref(mod);
  if( ret == KMOD_MODULE_LIVE)
	return(1);
  else
	return(0);
}

/** find which module is loaded 
 *
 * @return The name of the loaded module, or NULL if no modules are loaded.
 */
const char * usb_moded_find_module(void)
{
#ifdef NO_KMOD
  FILE *stream = 0;
  const char *result = 0;
  
  if( (stream = popen("lsmod", "r")) )
  {
    char *text = 0;
    size_t size = 0;
    
    while( getline(&text, &size, stream) >= 0 )
    {
      if( strstr(text, "g_nokia") )
      {
	result = MODULE_NETWORK;
	break;
      }
      if( strstr(text, "g_file_storage") )
      {
	result = MODULE_FILE_STORAGE;
	break;
      }
      if( strstr(text, "g_mass_storage") )
      {
	result = MODULE_MASS_STORAGE;
	break;
      }
     if( strstr(text, "g_ether") )
      {
	result = MODULE_WINDOWS_NET;
	break;
      }
      if( strstr(text, "g_ncm") )
      {
	result = "g_ncm";
	break;
      }
      if( strstr(text, "g_ffs") )
      {
	result = MODULE_MTP;
	break;
      }
      /* if switching without disconnect we might have some dynamic module loaded */
      if(strstr(text, get_usb_module()))
      {
	result = get_usb_module();
	break;
      }	
    }
    pclose(stream);
  }

return result;
#endif /* NO_KMOD */
  
  if(module_state_check("g_nokia"))
	return(MODULE_NETWORK);
  else if(module_state_check("g_ether"))
	return(MODULE_DEVELOPER); 
  else if(module_state_check("g_ncm"))
	return("g_ncm");
  else if(module_state_check("g_ffs"))
	return(MODULE_MTP);
  else if(module_state_check("g_mass_storage"))
	return(MODULE_MASS_STORAGE);
  else if(module_state_check("g_file_storage"))
	return(MODULE_FILE_STORAGE);
  else if(module_state_check(get_usb_module()))
	return(get_usb_module());
  /* no module loaded */
  return(0);
}

/** clean up for modules when usb gets disconnected
 *
 * @param module The name of the module to unload
 * @return 0 on success, non-zero on failure
 *
 */
int usb_moded_module_cleanup(const char *module)
{
  	int retry = 0, failure;

	if(!strcmp(module, MODULE_NONE))
		goto END;
	/* wait a bit for all components listening on dbus to clean up their act 
	sleep(2); */
	/* check if things were not reconnected in that timespan 
	if(get_usb_connection_state())
		return(0);
	*/
	
	failure = usb_moded_unload_module(module);

	/* if we have MODULE_MASS_STORAGE it might be MODULE_FILE_STORAGE might
	   be loaded. So check and unload that one if unloading fails first time */
	if(failure && !strcmp(MODULE_MASS_STORAGE, module))
		failure = usb_moded_unload_module(MODULE_FILE_STORAGE);

	/* if it still failed it might be the mode has not been cleaned-up correctly,
	   so we clean up the mode to be sure */
	if(failure)
	{
		usb_moded_mode_cleanup(usb_moded_find_module());
	}

	while(failure)
	{
		/* module did not get unloaded. We will wait a bit and try again */
		sleep(1);
		/* send the REALLY  disconnect message */
		usb_moded_send_signal(USB_REALLY_DISCONNECT);
		failure = usb_moded_unload_module(module);
		log_debug("unloading failure = %d\n", failure);
		if(!failure)
			break;
		if(!usb_moded_find_module())
			goto END;
		retry++;
		if(retry == 2)
			break;
	}
	if(!strcmp(module, MODULE_NETWORK))
	{
		if(retry >= 2)
		{	
			/* we exited the loop due to max retry's. Module is not unloaded yet
			   lets go for more extreme measures
			   lsof, then various options of kill
			*/
			log_info("Oh noes the platform is on fire!\n");
kill:		
			/* DIRTY DESPERATE WORKAROUND */
			/*system("for i in `lsof -t /dev/ttyGS*`; do cat /proc/$i/cmdline | sed 's/|//g' | sed "s/\x00/ /g" | awk '{ print $1 }' | xargs kill; done");
			system("for i in `ps ax | grep phonet-at | grep -v grep | awk '{ print $1 }'`; do kill -9 $i ; done");*/
			/* kill anything that still claims the usb tty's */
			system("for i in `lsof -t /dev/ttyGS*`; do kill -s SIGTERM $i ; done");
			system("for i in `lsof -t /dev/gc*`; do kill -s SIGTERM $i ; done");
			system("for i in `lsof -t /dev/mtp*`; do kill -s SIGTERM $i ; done");
			system("kill -s SIGTERM $(lsof -t /dev/usbacm");
		        // SP: three passes and for loops in sh?
		        // SP: system("kill -s SIGTERM $(lsof -t /dev/ttyGS* /dev/gc* /dev/mtp*") ?
			// SP: or popen + kill loop?
			/* try to unload again and give up if it did not work yet */
			failure = usb_moded_unload_module(module);
			if(failure && retry < 10)
			{
				retry++;
			  // SP: NOTE: we have a root process in busyloop sending kill signals to
			  // user processes? should we give them a chance to react?
				goto kill;
			  // SP: IMHO backwards goto is bad - a loop perhaps?
			}
			if(failure && retry == 10)
			{
				system("for i in `lsof -t /dev/ttyGS*`; do kill -9 $i ; done");
				system("for i in `lsof -t /dev/gc*`; do kill -9 $i ; done");
				system("for i in `lsof -t /dev/mtp*`; do kill -9 $i ; done");
				/* try again since there seem to be hard to kill processes there */
				system("killall -9 obexd");
				system("killall -9 msycnd");
				failure = usb_moded_unload_module(module);
			}

		}
	}
	if(!failure)
		log_info("Module %s unloaded successfully\n", module);
	else
	{
		log_err("Module %s did not unload! Failing and going to undefined.\n", module);
		return(1);
	}
END:
	return(0);
}

/** try to unload modules to support switching
 *
 *
 * @param force force unloading with a nasty clean-up on TRUE, or just try unloading when FALSE
 * @return 0 on success, 1 on failure, 2 if hard clean-up failed
 */

int usb_moded_module_switch_prepare (int force)
{
	const char *unload;
	int ret = 1; 

        unload = usb_moded_find_module();
        if(unload)
	{
		if(force)
			ret = usb_moded_module_cleanup(unload);
		else
                	ret = usb_moded_unload_module(unload);
	}
	if(ret && force)
		return(2);
	else 
		return(ret);
}

/** check for loaded modules and clean-up if they are not for the chosen mode 
 *
 * @param module_name  module name to check for
 *
 */
void check_module_state(const char *module_name)
{
  const char *module;

  module = usb_moded_find_module();
  if(module != NULL)
  {
        /* do nothing if the right module is already loaded */
        if(strcmp(usb_moded_find_module(), module_name) != 0)
        {
                log_debug("%s not loaded, cleaning up\n", module_name);
                usb_moded_module_switch_prepare(TRUE);
        }
  }
}


#ifdef NOKIA
gboolean usb_cleanup_timeout(gpointer data)
{
 	/* signal usb disconnected */
        usb_moded_send_signal(USB_DISCONNECTED);
	usb_moded_mode_cleanup(get_usb_module());
	usb_moded_module_cleanup(get_usb_module());
	set_usb_mode(MODE_UNDEFINED);
	return FALSE;
}
#endif /* NOKIA */

