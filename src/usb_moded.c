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
#include "usb_moded-gconf-private.h"
#include "usb_moded-modules.h"
#include "usb_moded-log.h"
#include "usb_moded-devicelock.h"
#include "usb_moded-modesetting.h"
#include "usb_moded-modules.h"
#include "usb_moded-appsync.h"

/* global definitions */

extern const char *log_name;
extern int log_level;
extern int log_type;

gboolean run_as_daemon = FALSE;
gboolean runlevel_ignore = FALSE;
struct usb_mode current_mode;
#ifdef NOKIA
gboolean special_mode = FALSE;
guint timeout_source = 0;
#endif /* NOKIA */

/*#ifdef APP_SYNC */
GList *applist = NULL;
/*#endif  APP_SYNC */

/* static helper functions */
static void usb_moded_init(void);
static gboolean charging_fallback(gpointer data);
static void usage(void);
static gboolean daemonize(void);


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
  	current_mode.connected = TRUE;
	set_usb_connected_state();
  }
  else
  {
  	current_mode.connected = FALSE;
  	/* signal usb disconnected */
	log_debug("usb disconnected\n");
	/* clean-up state */
	usb_moded_mode_cleanup(get_usb_module());
	usb_moded_send_signal(USB_DISCONNECTED);
#ifdef NOKIA
	timeout_source = g_timeout_add_seconds(5, usb_module_timeout_cleanup, NULL);
#else
	/* unload modules and general cleanup */
	usb_moded_module_cleanup(get_usb_module());
#endif /* NOKIA */
	
	set_usb_mode(MODE_UNDEFINED);
  }
}

/** set the chosen usb state
 *
 */
void set_usb_connected_state(void)
{	

  const char *mode_to_set;  
#ifdef NOKIA
  int export = 0, act_dead = 0;
#endif /* NOKIA */

  /* signal usb connected */
  log_debug("usb connected\n");
  usb_moded_send_signal(USB_CONNECTED);
  mode_to_set = get_mode_setting();
#ifdef NOKIA
  /* check if we are allowed to export system contents 0 is unlocked */
  export = usb_moded_get_export_permission();
  /* check if we are in acting dead or not, /tmp/USER will not exist in acting dead */
  act_dead = access("/tmp/USER", R_OK);
  if(mode_to_set && !export && !act_dead)
#else
  if(mode_to_set)
#endif /* NOKIA */
  {
	if(!strcmp(MODE_ASK, mode_to_set))
	{
		/* send signal, mode will be set when the dialog service calls
	  	 the set_mode method call.
	 	*/
		usb_moded_send_signal(USB_CONNECTED_DIALOG_SHOW);
		/* fallback to charging mode after 3 seconds */
		g_timeout_add_seconds(3, charging_fallback, NULL);
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
  
}

/** set the usb mode 
 *
 * @param mode The requested USB mode
 * 
 */
void set_usb_mode(const char *mode)
{
  int ret=0, net=0;
  
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
  if(!strcmp(mode, MODE_CHARGING))
  {
	check_module_state(MODULE_MASS_STORAGE);
	/* for charging we use a fake file_storage (blame USB certification for this insanity */
	set_usb_module(MODULE_MASS_STORAGE);
	/* MODULE_CHARGING has all the parameters defined, so it will not match the g_file_storage rule in usb_moded_load_module */
	ret = usb_moded_load_module(MODULE_CHARGING);
	goto end;
  }

#ifdef N900 
  if(!strcmp(mode, MODE_OVI_SUITE))
  {
	check_module_state(MODULE_NETWORK);
 	set_usb_module(MODULE_NETWORK);
	ret = usb_moded_load_module(MODULE_NETWORK_MTP);
	if(!ret)
		ret = set_ovi_suite_mode(applist);
  } 
#endif /* N900 */

  if(!strcmp(mode, MODE_WINDOWS_NET))
  {	
	check_module_state(MODULE_WINDOWS_NET);
	set_usb_module(MODULE_WINDOWS_NET);
	ret = usb_moded_load_module(MODULE_WINDOWS_NET);
	net = system("ifdown usb0 ; ifup usb0");
  }

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

/** get the usb mode 
 *
 * @return the currently set mode
 *
 */
const char * get_usb_mode(void)
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
const char * get_usb_module(void)
{
  return(current_mode.module);
}

/** get if the cable is connected or not
 *
 * @ return A boolean value for connected (TRUE) or not (FALSE)
 *
 */
gboolean get_usb_connection_state(void)
{
	return current_mode.connected;
}


/*================  Static functions ===================================== */

/* set default values for usb_moded */
static void usb_moded_init(void)
{
#ifdef NOKIA
  char readbuf[MAX_READ_BUF];
  FILE *proc_fd;
#endif /* NOKIA */

  current_mode.connected = FALSE;
  current_mode.mounted = FALSE;
  current_mode.mode = strdup(MODE_UNDEFINED);
  current_mode.module = g_strdup(MODULE_NONE);

#ifdef NOKIA
  proc_fd = fopen("/proc/cmdline", "r");
  if(proc_fd)
  {
    fgets(readbuf, MAX_READ_BUF, proc_fd);
    readbuf[strlen(readbuf)-1]=0;

    if(strstr(readbuf, "nsu"))
	special_mode = TRUE;
    fclose(proc_fd);
  }
	
#endif /* NOKIA */
#ifdef APP_SYNC
  applist = readlist();
#endif /* APP_SYNC */
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
                  "  -d,  --daemon	  run as a daemon\n"
                  "  -s,  --force-syslog  log to syslog even when not "
                  "daemonized\n"
                  "  -T,  --force-stderr  log to stderr even when daemonized\n"
                  "  -D,  --debug	  turn on debug printing\n"
                  "  -h,  --help          display this help and exit\n"
                  "  -v,  --version       output version information and exit\n"
		  "  -w,  --watch-off	  do not act on runlevel change\n"
                  "\n");
}

/* Turn the process in a daemon */
static gboolean daemonize(void)
{
        gint retries = 0;
        gint i = 0;
        gchar str[10];

        if (getppid() == 1)
                exit(0);      /* Already daemonized */

        /* Detach from process group */
        switch (fork()) 
	{
        case -1:
                /* Failure */
                exit(1);

        case 0:
                /* Child */
                break;

        default:
                /* Parent -- exit */
                exit(0);
        }

        /* Detach TTY */
        setsid();

        /* Close all file descriptors and redirect stdio to /dev/null */
        if ((i = getdtablesize()) == -1)
                i = 256;

        while (--i >= 0) 
	{
                if (close(i) == -1) 
		{
                        if (retries > 10) 
			{
                                log_crit("close() was interrupted more than 10 times. Exiting.\n");
                                exit(1);
                        }

                        if (errno == EINTR) 
			{
                                log_err("close() was interrupted; retrying.\n");
                                errno = 0;
                                i++;
                                retries++;
                        } 
			else if (errno == EBADF) 
			{
                               /* fprintf(stdout, "Failed to close() fd %d; %s. Ignoring.\n",
                                        i + 1, g_strerror(errno));*/
                                errno = 0;
                        } 
			else 
			{
                                log_crit("Failed to close() fd %d; %s. Exiting.", i + 1, g_strerror(errno));
                                exit(1);
                        }
                } 
		else 
        	        retries = 0;
	}

        if ((i = open("/dev/null", O_RDWR)) == -1) 
	{
                log_crit("Cannot open `/dev/null'; %s. Exiting.", g_strerror(errno));
                exit(1);
        }

        if ((dup(i) == -1)) 
	{
                log_crit("Failed to dup() `/dev/null'; %s. Exiting.", g_strerror(errno));
                exit(1);
        }
        if ((dup(i) == -1)) 
	{
		log_crit("Failed to dup() `/dev/null'; %s. Exiting.", g_strerror(errno));
                exit(1);
        }

        /* Set umask */
        umask(022);

        /* Set working directory */
        if ((chdir("/tmp") == -1)) 
	{
                log_crit("Failed to chdir() to `/tmp'; %s. Exiting.", g_strerror(errno));
                exit(1);
        }

        /* Single instance */
        if ((i = open(USB_MODED_LOCKFILE, O_RDWR | O_CREAT, 0640)) == -1) 
	{
                log_crit("Cannot open lockfile; %s. Exiting.", g_strerror(errno));
                exit(1);
        }

        if (lockf(i, F_TLOCK, 0) == -1) 
	{
                log_crit("Already running. Exiting.");
                exit(1);
        }

        sprintf(str, "%d\n", getpid());
        write(i, str, strlen(str));
        close(i);

        /* Ignore TTY signals */
        signal(SIGTSTP, SIG_IGN);
        signal(SIGTTOU, SIG_IGN);
        signal(SIGTTIN, SIG_IGN);

        /* Ignore child terminate signal */
        signal(SIGCHLD, SIG_IGN);

        log_info("Daemon running.");
        return 0;
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
        while ((opt = getopt_long(argc, argv, "dsTDhvw", options, &opt_idx)) != -1) 
	{
                switch (opt) 
		{
                	case 'd':
                        	run_as_daemon = TRUE;
	                        break;

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

        if(run_as_daemon == TRUE)
	{
		log_notice("going to daemon mode\n");
		daemonize();
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
#ifdef NOKIA
	start_devicelock_listener();
	if(!runlevel_ignore)
		usb_moded_dsme_listener();
#endif /* NOKIA */

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
