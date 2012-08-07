/**
  @file usb_moded.c

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
#include <getopt.h>

#include <sys/stat.h>
#include <sys/wait.h>
#ifdef NOKIA
#include <string.h>
#endif

#include "usb_moded.h"
#include "usb_moded-modes.h"
#include "usb_moded-dbus.h"
#include "usb_moded-dbus-private.h"
#include "usb_moded-hw-ab.h"
#include "usb_moded-gconf.h"
#include "usb_moded-modules.h"
#include "usb_moded-log.h"
#include "usb_moded-lock.h"
#include "usb_moded-modesetting.h"
#include "usb_moded-modules.h"
#include "usb_moded-appsync.h"
#include "usb_moded-trigger.h"
#include "usb_moded-config.h"
#include "usb_moded-config-private.h"
#include "usb_moded-network.h"

/* global definitions */

extern const char *log_name;
extern int log_level;
extern int log_type;

gboolean runlevel_ignore = FALSE;
struct usb_mode current_mode;
guint charging_timeout = 0;
#ifdef NOKIA
gboolean special_mode = FALSE;
guint timeout_source = 0;
#endif /* NOKIA */
#ifdef APP_SYNC
static GList *modelist;
#endif /* APP_SYNC */

/* static helper functions */
static gboolean set_disconnected(gpointer data);
static void usb_moded_init(void);
static gboolean charging_fallback(gpointer data);
static void usage(void);


/* ============= Implementation starts here =========================================== */

/** set the usb connection status 
 *
 * @param connected The connection status, true for connected
 * 
 */
void set_usb_connected(gboolean connected)
{
#ifdef NOKIA
  if(special_mode)
  {
	/* Do nothing for the time being to leave currently loaded modules active.
	   Set special_mode to false so next usb connect makes things work as they should.
	*/
	special_mode = FALSE;
	log_debug("nsu active. Not doing anything before cable disconnect/reconnect\n");
	return;
  }
#endif /* NOKIA */
	
  if(connected)
  {
 	/* do not go through the routine if already connected to avoid
           spurious load/unloads due to faulty signalling 
	   NOKIA: careful with devicelock
	*/
	if(current_mode.connected)
		return;

#ifdef NOKIA
	if(timeout_source)
	{
		g_source_remove(timeout_source);
		timeout_source = 0;
	}
#endif /* NOKIA */	
	if(charging_timeout)
	{
		g_source_remove(charging_timeout);
		charging_timeout = 0;
	}
  	current_mode.connected = TRUE;
	set_usb_connected_state();
  }
  else
  {
	current_mode.connected = FALSE;
	set_disconnected(NULL);
  }		

}

static gboolean set_disconnected(gpointer data)
{
  /* only disconnect for real if we are actually still disconnected */
  if(!get_usb_connection_state())
	{
		log_debug("usb disconnected\n");
#ifdef NOKIA
		/* delayed clean-up of state */
		timeout_source = g_timeout_add_seconds(3, usb_cleanup_timeout, NULL);
#else
  		/* signal usb disconnected */
		usb_moded_send_signal(USB_DISCONNECTED);
		/* unload modules and general cleanup */
		usb_moded_mode_cleanup(get_usb_module());
		usb_moded_module_cleanup(get_usb_module());
		set_usb_mode(MODE_UNDEFINED);
#endif /* NOKIA */
	
	}
  return FALSE;
}

/** set the chosen usb state
 *
 */
void set_usb_connected_state(void)
{	

  const char *mode_to_set;  
#ifdef MEEGOLOCK
  int export = 0;
#endif /* MEEGOLOCK */

  /* signal usb connected */
  log_debug("usb connected\n");
  usb_moded_send_signal(USB_CONNECTED);
  mode_to_set = get_mode_setting();
#ifdef MEEGOLOCK
  /* check if we are allowed to export system contents 0 is unlocked */
  export = usb_moded_get_export_permission();
#endif
#ifdef MEEGOLOCK
  int act_dead = 0;
  /* check if we are in acting dead or not, /tmp/USER will not exist in acting dead */
  act_dead = access("/tmp/USER", R_OK);
  if(mode_to_set && !export && !act_dead)
#else
  if(mode_to_set)
#endif /* MEEGOLOCK */
  {
#ifdef NOKIA
	/* If we switch to another mode than the one that is still set before the 
	   clean-up timeout expired we need to clean up */
	if(strcmp(mode_to_set, get_usb_mode()))
		 usb_moded_mode_cleanup(get_usb_module());
#endif /* NOKIA */

	if(!strcmp(MODE_ASK, mode_to_set))
	{
		/* send signal, mode will be set when the dialog service calls
	  	 the set_mode method call.
	 	*/
		usb_moded_send_signal(USB_CONNECTED_DIALOG_SHOW);
		/* fallback to charging mode after 3 seconds */
		charging_timeout = g_timeout_add_seconds(3, charging_fallback, NULL);
		/* in case there was nobody listening for the UI, they will know 
		   that the UI is needed by requesting the current mode */
		set_usb_mode(MODE_ASK);
	}
	else
		set_usb_mode(mode_to_set);
  }
  else
  {
	/* gconf is corrupted or does not return a value, fallback to charging 
	   We also fall back here in case the device is locked and we do not 
	   export the system contents. Or if we are in acting dead mode.
	*/
	set_usb_mode(MODE_CHARGING);
  }
  free((void *)mode_to_set); 
}

/** set the usb mode 
 *
 * @param mode The requested USB mode
 * 
 */
void set_usb_mode(const char *mode)
{
  /* set return to 1 to be sure to error out if no matching mode is found either */
  int ret=1, net=0;
  
  if(!strcmp(mode, MODE_MASS_STORAGE))
  {

	check_module_state(MODULE_MASS_STORAGE);
	/* now proceed to set the mode correctly */
  	set_usb_module(MODULE_MASS_STORAGE);
	ret = usb_moded_load_module(MODULE_MASS_STORAGE);
	if(!ret)
        	ret = set_mass_storage_mode();
	goto end;
  }
  else if(!strcmp(mode, MODE_CHARGING))
  {
	check_module_state(MODULE_MASS_STORAGE);
	/* for charging we use a fake file_storage (blame USB certification for this insanity */
	set_usb_module(MODULE_MASS_STORAGE);
	/* MODULE_CHARGING has all the parameters defined, so it will not match the g_file_storage rule in usb_moded_load_module */
	ret = usb_moded_load_module(MODULE_CHARGING);
	goto end;
  }
else if(!strcmp(mode, MODE_DEVELOPER))
  {
	check_module_state(MODULE_DEVELOPER);
	set_usb_module(MODULE_DEVELOPER);
	ret = usb_moded_load_module(MODULE_DEVELOPER);
	net = usb_network_up();	
	goto end;
  }
#ifdef N900 
  else if(!strcmp(mode, MODE_OVI_SUITE))
  {
	check_module_state(MODULE_NETWORK);
 	set_usb_module(MODULE_NETWORK);
	ret = usb_moded_load_module(MODULE_NETWORK_MTP);
	if(!ret)
		ret = set_ovi_suite_mode();
	goto end;
  } 
#endif /* N900 */

  else if(!strcmp(mode, MODE_WINDOWS_NET))
  {	
	check_module_state(MODULE_WINDOWS_NET);
	set_usb_module(MODULE_WINDOWS_NET);
	ret = usb_moded_load_module(MODULE_WINDOWS_NET);
	net = usb_network_up();	
	goto end;
  }
  else if(!strcmp(mode, MODE_MTP))
  {
	check_module_state(MODULE_MTP);
	set_usb_module(MODULE_MTP);
	ret = usb_moded_load_module(MODULE_MTP);
	goto end;
  }
  else if(!strcmp(mode, MODE_ASK))
  {
	ret = 0;
  }

#ifdef DYN_MODE 
  /* go through all the dynamic modes if the modelist exists*/
  if(modelist)
  {
    GList *iter;

    for( iter = modelist; iter; iter = g_list_next(iter) )
    {
      struct mode_list_elem *data = iter->data;
      if(!strcmp(mode, data->mode_name))
      {
  	check_module_state(data->mode_module);
	set_usb_module(data->mode_module);
	ret = usb_moded_load_module(data->mode_module);
        ret = set_dynamic_mode(data);
      }
    }
  }
#endif /* DYN_MODE */

end:
  /* if ret != 0 then usb_module loading failed */
  if(ret)
  {
	  set_usb_module(MODULE_NONE);
	  mode = MODE_UNDEFINED;
  }
  free(current_mode.mode);
  current_mode.mode = strdup(mode);
  usb_moded_send_signal(get_usb_mode());
}
/** check if a given usb_mode exists
 *
 * @param mode The mode to look for
 * @return 0 if mode exits, 1 if it does not exist
 *
 */
int valid_mode(const char *mode)
{

  if(!strcmp(MODE_MASS_STORAGE, mode) || !strcmp(MODE_OVI_SUITE, mode) || !strcmp(MODE_CHARGING, mode) 
     || !strcmp(MODE_WINDOWS_NET, mode) || !strcmp(MODE_DEVELOPER,mode) || !strcmp(MODE_MTP,mode))
	return(0);
  else
#ifdef DYN_MODE
  {
    /* check dynamic modes */
    if(modelist)
    {
      GList *iter;

      for( iter = modelist; iter; iter = g_list_next(iter) )
      {
        struct mode_list_elem *data = iter->data;
	if(!strcmp(mode, data->mode_name))
		return(0);
      }
    }
  }
#endif /* DYN_MODE */
  return(1);

}

/** get the usb mode 
 *
 * @return the currently set mode
 *
 */
inline const char * get_usb_mode(void)
{
  return(current_mode.mode);
}

/** set the loaded module 
 *
 * @param module The module name for the requested mode
 *
 */
void set_usb_module(const char *module)
{
  free(current_mode.module);
  current_mode.module = strdup(module);
}

/** get the supposedly loaded module
 *
 * @return The name of the loaded module
 *
 */
inline const char * get_usb_module(void)
{
  return(current_mode.module);
}

/** get if the cable is connected or not
 *
 * @ return A boolean value for connected (TRUE) or not (FALSE)
 *
 */
inline gboolean get_usb_connection_state(void)
{
	return current_mode.connected;
}

/** set connection status for some corner cases
 *
 * @param state The connection status that needs to be set. Connected (TRUE)
 *
 */
inline void set_usb_connection_state(gboolean state)
{
	current_mode.connected = state;
}

/*================  Static functions ===================================== */

/* set default values for usb_moded */
static void usb_moded_init(void)
{
#ifdef NOKIA_NSU
  char readbuf[MAX_READ_BUF];
  FILE *proc_fd;
#endif /* NOKIA_NSU */

  current_mode.connected = FALSE;
  current_mode.mounted = FALSE;
  current_mode.mode = strdup(MODE_UNDEFINED);
  current_mode.module = strdup(MODULE_NONE);

#ifdef NOKIA_NSU
  proc_fd = fopen("/proc/cmdline", "r");
  if(proc_fd)
  {
    fgets(readbuf, MAX_READ_BUF, proc_fd);
    readbuf[strlen(readbuf)-1]=0;

    if(strstr(readbuf, "nsu"))
	special_mode = TRUE;
    fclose(proc_fd);
  }
	
#endif /* NOKIA_NSU */
#ifdef APP_SYNC
  readlist();
#endif /* APP_SYNC */
#ifdef DYN_MODE
  modelist = read_mode_list();
#endif /* DYN_MODE */

#ifdef UDEV
  if(check_trigger())
	trigger_init();
#endif /* UDEV */
  /* TODO: add more start-up clean-up and init here if needed */
}	

/* charging fallback handler */
static gboolean charging_fallback(gpointer data)
{
  /* if a mode has been set we don't want it changed to charging
   * after 5 seconds. We set it to ask, so anything different 
   * means a mode has been set */
  if(strcmp(get_usb_mode(), MODE_ASK) != 0)
		  return FALSE;
  set_usb_mode(MODE_CHARGING);
  /* since this is the fallback, we keep an indication
     for the UI, as we are not really in charging mode.
  */
  free(current_mode.mode);
  current_mode.mode = strdup(MODE_ASK);
  charging_timeout = 0;
  log_info("Falling back on charging mode.\n");
	
  return(FALSE);
}

/* Display usage information */
static void usage(void)
{
        fprintf(stdout,
                "Usage: usb_moded [OPTION]...\n"
                  "USB mode daemon\n"
                  "\n"
                  "  -s,  --force-syslog  log to syslog\n"
                  "  -T,  --force-stderr  log to stderr\n"
                  "  -D,  --debug	  turn on debug printing\n"
                  "  -h,  --help          display this help and exit\n"
                  "  -v,  --version       output version information and exit\n"
		  "  -w,  --watch-off	  do not act on runlevel change\n"
                  "\n");
}

int main(int argc, char* argv[])
{
	int result = EXIT_FAILURE;
        int opt = 0, opt_idx = 0;
	GMainLoop *mainloop = NULL;

	struct option const options[] = {
                { "daemon", no_argument, 0, 'd' },
                { "force-syslog", no_argument, 0, 's' },
                { "force-stderr", no_argument, 0, 'T' },
                { "debug", no_argument, 0, 'D' },
                { "help", no_argument, 0, 'h' },
                { "version", no_argument, 0, 'v' },
                { 0, 0, 0, 0 }
        };

	log_name = basename(*argv);

	 /* Parse the command-line options */
        while ((opt = getopt_long(argc, argv, "sTDhvw", options, &opt_idx)) != -1) 
	{
                switch (opt) 
		{
		        case 's':
                        	log_type = LOG_TO_SYSLOG;
                        	break;

                	case 'T':
                        	log_type = LOG_TO_STDERR;
                        	break;

                	case 'D':
                        	log_level = LOG_DEBUG;
                        	break;

                	case 'h':
                        	usage();
				exit(0);

	                case 'v':
				printf("USB mode daemon version: %s\n", VERSION);
				exit(0);

			case 'w':
				runlevel_ignore = TRUE;
				printf("Ignore runlevel changes.\n");
				break;
	                default:
        	                usage();
				exit(0);
                }
        }

	/* silence system() calls */
	if(log_type != LOG_TO_STDERR || log_level != LOG_DEBUG )	
	{
		freopen("/dev/null", "a", stdout);
		freopen("/dev/null", "a", stderr);
	}
        g_type_init();
	g_thread_init(NULL);
	mainloop = g_main_loop_new(NULL, FALSE);
	log_debug("usb_moded starting\n");

	/* init daemon into a clean state first, then dbus and hw_abstraction last */
	usb_moded_init();
	if( !usb_moded_dbus_init() )
	{
		log_crit("dbus ipc init failed\n");
		goto EXIT;
	}
	if( !hwal_init() )
	{
		log_crit("hwal init failed\n");
		goto EXIT;
	}
#ifdef MEEGOLOCK
	start_devicelock_listener();
	if(!runlevel_ignore)
		usb_moded_dsme_listener();
#endif /* MEEGOLOCK */

	/* init succesful, run main loop */
	result = EXIT_SUCCESS;  
	g_main_loop_run(mainloop);
EXIT:
	/* exiting and clean-up when mainloop ended */
	hwal_cleanup();
	usb_moded_dbus_cleanup();

	/* If the mainloop is initialised, unreference it */
        if (mainloop != NULL)
                g_main_loop_unref(mainloop);

	return result;
}
