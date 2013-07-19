/**
  @file usb_moded-modesetting.c
 
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
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>

#include <glib.h>

#include "usb_moded.h"
#include "usb_moded-modules.h"
#include "usb_moded-modes.h"
#include "usb_moded-log.h"
#include "usb_moded-dbus.h"
#include "usb_moded-dbus-private.h"
#include "usb_moded-appsync.h"
#include "usb_moded-config.h"
#include "usb_moded-modesetting.h"
#include "usb_moded-network.h"
#include "usb_moded-upstart.h"

static void report_mass_storage_blocker(const char *mountpoint, int try);

#ifdef MAYBE_NEEDED
int find_number_of_mounts(void)
{
  const char *mount;
  gchar **mounts;
  int mountpoints = 0, i = 0;
	
  mount = find_mounts();
  if(mount)
  {
  	mounts = g_strsplit(mount, ",", 0);
        /* check amount of mountpoints */
        for(i=0 ; mounts[i] != NULL; i++)
        {
        	mountpoints++;
        }
  }
  return(mountpoints);
}
#endif /* MAYBE_NEEDED */

int write_to_file(const char *path, const char *text)
{
  int err = -1;
  int fd = -1;
  size_t todo = strlen(text);

  /* no O_CREAT -> writes only to already existing files */
  if( (fd = TEMP_FAILURE_RETRY(open(path, O_WRONLY))) == -1 )
  {
    /* gcc -pedantic does not like "%m"
       log_warning("open(%s): %m", path); */
    log_warning("open(%s): %s", path, strerror(errno));
    goto cleanup;
  }

  while( todo > 0 )
  {
    ssize_t n = TEMP_FAILURE_RETRY(write(fd, text, todo));
    if( n < 0 )
    {
      log_warning("write(%s): %s", path, strerror(errno));
      goto cleanup;
    }
    todo -= n;
    text += n;
  }

  err = 0;

cleanup:

  if( fd != -1 ) TEMP_FAILURE_RETRY(close(fd));

  return err;
}


static int set_mass_storage_mode(void)
{
        gchar *command;
        char command2[256];
        const char *mount;
        gchar **mounts;
        int ret = 0, i = 0, mountpoints = 0, fua = 0, try = 0;

        /* send unmount signal so applications can release their grasp on the fs, do this here so they have time to act */
        usb_moded_send_signal(USB_PRE_UNMOUNT);
        fua = find_sync();
        mount = find_mounts();
        if(mount)
        {
        	mounts = g_strsplit(mount, ",", 0);
             	/* check amount of mountpoints */
                for(i=0 ; mounts[i] != NULL; i++)
                {
                	mountpoints++;
                }

                /* check if the file storage module has been loaded with sufficient luns in the parameter,
                if not, unload and reload or load it. Since  mountpoints start at 0 the amount of them is one more than their id */
                sprintf(command2, "/sys/devices/platform/musb_hdrc/gadget/gadget-lun%d/file", (mountpoints - 1) );
                if(access(command2, R_OK) == -1)
                {
                	log_debug("%s does not exist, unloading and reloading mass_storage\n", command2);
                        usb_moded_unload_module(MODULE_MASS_STORAGE);
                        sprintf(command2, "modprobe %s luns=%d \n", MODULE_MASS_STORAGE, mountpoints);
                        log_debug("usb-load command = %s \n", command2);
                        ret = system(command2);
                        if(ret)
                        	return(ret);
                }
                /* umount filesystems */
                for(i=0 ; mounts[i] != NULL; i++)
                {
                	/* check if filesystem is mounted or not, if ret = 1 it is already unmounted */
umount:                 command = g_strconcat("mount | grep ", mounts[i], NULL);
                        ret = system(command);
                        g_free(command);
                        if(!ret)
                        {
				/* no check for / needed as that will fail to umount anyway */
                        	command = g_strconcat("umount ", mounts[i], NULL);
                                log_debug("unmount command = %s\n", command);
                                ret = system(command);
                                g_free(command);
                                if(ret != 0)
                                {
					if(try != 3)
					{
						try++;
						sleep(1);
						log_err("Umount failed. Retrying\n");
						report_mass_storage_blocker(mount, 1);
						goto umount;
					}
					else
					{
                                		log_err("Unmounting %s failed\n", mount);
						report_mass_storage_blocker(mount, 2);
#ifdef NOKIA
                                        	usb_moded_send_error_signal("qtn_usb_filessystem_inuse");
#else
                                        	usb_moded_send_error_signal(UMOUNT_ERROR);
#endif
     	                                   	return(ret);
					}
                                }
                         }
                         else
                         	/* already unmounted. Set return value to 0 since there is no error */
                                ret = 0;
              	}
		
	        /* activate mounts after sleeping 1s to be sure enumeration happened and autoplay will work in windows*/
		usleep(1800);
                for(i=0 ; mounts[i] != NULL; i++)
                {       
			sprintf(command2, "echo %i  > /sys/devices/platform/musb_hdrc/gadget/gadget-lun%d/nofua", fua, i);
                        log_debug("usb lun = %s active\n", command2);
                        system(command2);
                	sprintf(command2, "/sys/devices/platform/musb_hdrc/gadget/gadget-lun%d/file", i);
                        log_debug("usb lun = %s active\n", command2);
 			write_to_file(command2, mounts[i]);
                }
                g_strfreev(mounts);
		g_free((gpointer *)mount);
	}

	/* only send data in use signal in case we actually succeed */
        if(!ret)
                usb_moded_send_signal(DATA_IN_USE);
	
	return(ret);

}

static int unset_mass_storage_mode(void)
{
        gchar *command;
        char command2[256];
        const char *mount;
        gchar **mounts;
        int ret = 1, i = 0;

        mount = find_mounts();
        if(mount)
        {
        	mounts = g_strsplit(mount, ",", 0);
                for(i=0 ; mounts[i] != NULL; i++)
                {
                	/* check if it is still or already mounted, if so (ret==0) skip mounting */
                	command = g_strconcat("mount | grep ", mounts[i], NULL);
                        ret = system(command);
                        g_free(command);
                        if(ret)
                        {
                        	command = g_strconcat("mount ", mounts[i], NULL);
                                ret = system(command);
                                g_free(command);
                                if(ret != 0)
                                {
                                	log_err("Mounting %s failed\n", mount);
                                        usb_moded_send_error_signal(RE_MOUNT_FAILED);
					g_free((gpointer *)mount);
                                        mount = find_alt_mount();
                                        if(mount)
                                        {
						/* check if it is already mounted, if not mount failure fallback */
                                                command = g_strconcat("mount | grep ", mount, NULL);
                                                ret = system(command);
                                                g_free(command);
                                                if(ret)
                                                {
                                               		command = g_strconcat("mount -t tmpfs tmpfs -o ro --size=512K ", mount, NULL);
							log_debug("Total failure, mount ro tmpfs as fallback\n");
                                                        ret = system(command);
                                                        g_free(command);
                                                }

                                        }

                                  }
                        }

			sprintf(command2, "echo \"\"  > /sys/devices/platform/musb_hdrc/gadget/gadget-lun%d/file", i);
                	log_debug("usb lun = %s inactive\n", command2);
                	system(command2);
                 }
                 g_strfreev(mounts);
		 g_free((gpointer *)mount);
        }

	return(ret);

}

static void report_mass_storage_blocker(const char *mountpoint, int try)
{
  FILE *stream = 0;
  gchar *lsof_command = 0;
  int count = 0;

  lsof_command = g_strconcat("lsof ", mountpoint, NULL);

  if( (stream = popen(lsof_command, "r")) )
  {
    char *text = 0;
    size_t size = 0;

    while( getline(&text, &size, stream) >= 0 )
    {
        /* skip the first line as it does not contain process info */
        if(count != 0)
        {
          gchar **split = 0;
          split = g_strsplit((const gchar*)text, " ", 2);
          log_err("Mass storage blocked by process %s\n", split[0]);
          usb_moded_send_error_signal(split[0]);
          g_strfreev(split);
        }
        count++;
    }
    pclose(stream);
  }
  g_free(lsof_command);
  if(try == 2)
	log_err("Setting Mass storage blocked. Giving up.\n");

}

#if 0
/* NOT NEEDED ANYMORE?  : clean up buteo-mtp hack */
int set_mtp_mode(void)
{
  mkdir("/dev/mtp", S_IRWXO|S_IRWXU);
  system("mount -t functionfs mtp  -o gid=10000,mode=0770 /dev/mtp\n");	
  system("buteo-mtp start\n");

  return 0;
}
#endif

#ifdef N900
int set_ovi_suite_mode(void)
{
#ifdef NOKIA
   int timeout = 1;
#endif /* NOKIA */

  
#ifdef APP_SYNC
  activate_sync(MODE_OVI_SUITE);
#else
  write_to_file("/sys/devices/platform/musb_hdrc/gadget/softconnect", "1");
#endif /* APP_SYNC */
  /* bring network interface up in case no other network is up */
#ifdef DEBIAN
  system("ifdown usb0 ; ifup usb0");
#else
  usb_network_down(NULL);
  usb_network_up(NULL);
#endif /* DEBIAN */

#ifdef NOKIA
  /* timeout for exporting CDROM image */
  timeout = find_cdrom_timeout();
  if(timeout == 0)
	timeout = 1;
  g_timeout_add_seconds(timeout, export_cdrom, NULL);
#endif /* NOKIA */

  return(0);
}
#endif /* N900 */

int set_dynamic_mode(void)
{

  struct mode_list_elem *data; 

  data = get_usb_mode_data();

  if(!data)
	return 1;

  if(!strcmp(data->mode_name, MODE_MASS_STORAGE))
  {
	return set_mass_storage_mode();
  }

#ifdef APP_SYNC
  if(data->appsync)
  	activate_sync(data->mode_name);
#endif
  /* make sure things are disabled before changing functionality */
  if(data->softconnect_disconnect)
  {
	write_to_file(data->softconnect_path, data->softconnect_disconnect);
  }
  /* set functionality first, then enable */
  if(data->sysfs_path)
  {
	write_to_file(data->sysfs_path, data->sysfs_value);
	log_debug("writing to file %s, value %s\n", data->sysfs_path, data->sysfs_value);
  }
  if(data->android_extra_sysfs_value && data->android_extra_sysfs_path)
  {
	write_to_file(data->android_extra_sysfs_path, data->android_extra_sysfs_value);
  }
  if(data->softconnect)
  {
	write_to_file(data->softconnect_path, data->softconnect);
  }

  /* functionality should be enabled, so we can enable the network now */
  if(data->network)
  {
#ifdef DEBIAN
  	char command[256];

	g_snprintf(command, 256, "ifdown %s ; ifup %s", data->network_interface, data->network_interface);
        system(command);
#else
	usb_network_down(data);
	usb_network_up(data);
#endif /* DEBIAN */
  }
  return(0);
}

void unset_dynamic_mode(void)
{ 

  struct mode_list_elem *data; 

  data = get_usb_mode_data();

  /* the modelist could be empty */
  if(!data)
	return;

  if(!strcmp(data->mode_name, MODE_MASS_STORAGE))
  {
	set_mass_storage_mode();
	return;
  }

  /* disconnect before changing functionality */
  if(data->softconnect)
  {
	write_to_file(data->softconnect_path, data->softconnect_disconnect);
  }
  if(data->sysfs_path)
  {
	write_to_file(data->sysfs_path, data->sysfs_reset_value);
	log_debug("writing to file %s, value %s\n", data->sysfs_path, data->sysfs_reset_value);
  }

}

#ifdef NOKIA
gboolean export_cdrom(gpointer data)
{
  const char *path = NULL;

  path = find_cdrom_path();

  if(path == NULL)
  {
	log_debug("No cdrom path specified => not exporting.\n");
	return(FALSE);
  }
  if(access(path, F_OK) == 0)
  {
        write_to_file("/sys/devices/platform/musb_hdrc/gadget/lun0/file", path);
  }
  else
	log_debug("Cdrom image file does not exist => no export.\n");
  g_free((gpointer *)path);

  return(FALSE);
}

#endif /* NOKIA */


/** clean up mode changes or extra actions to perform after a mode change 
 * @param module Name of module currently in use
 * @return 0 on success, non-zero on failure
 *
 */
int usb_moded_mode_cleanup(const char *module)
{

	log_debug("Cleaning up mode\n");

#ifdef APP_SYNC
	appsync_stop();
#endif /* APP_SYNC */

        if(!strcmp(module, MODULE_MASS_STORAGE)|| !strcmp(module, MODULE_FILE_STORAGE))
        {
		/* no clean-up needs to be done when we come from charging mode. We need
		   to check since we use fake mass-storage for charging */
		if(!strcmp(MODE_CHARGING, get_usb_mode()))
		  return 0;	

		unset_mass_storage_mode();
        }
#ifdef N900
        if(!strcmp(module, MODULE_NETWORK))
        {
                /* preventive sync in case of bad quality mtp clients */
                sync();
                /* bring network down immediately */
                /*system("ifdown usb0"); */
		usb_network_down(NULL);
                /* do soft disconnect 
  		write_to_file("/sys/devices/platform/musb_hdrc/gadget/softconnect", "0"); */
		/* DIRTY WORKAROUND: acm/phonet does not work as it should, remove when it does */
		system("killall -SIGTERM acm");
        }
#endif /* N900 */
	if(!strcmp(module, MODULE_MTP))
	{
		/* stop service before umounting ;) */
  		system("buteo-mtp stop\n");
		system("umount /dev/mtp");
	}

	if(get_usb_mode_data())
		unset_dynamic_mode();

        return(0);
}

