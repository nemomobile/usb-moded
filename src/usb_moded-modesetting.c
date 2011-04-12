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

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include <glib.h>

#include "usb_moded.h"
#include "usb_moded-modules.h"
#include "usb_moded-log.h"
#include "usb_moded-dbus.h"
#include "usb_moded-dbus-private.h"
#include "usb_moded-appsync.h"
#include "usb_moded-config.h"
#include "usb_moded-modesetting.h"

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

int set_mass_storage_mode(void)
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
                        	command = g_strconcat("umount ", mounts[i], NULL);
                                log_debug("unmount command = %s\n", command);
                                ret = system(command);
                                g_free(command);
                                if(ret != 0)
                                {
					if(try != 1)
					{
						try++;
						sleep(1);
						log_err("Umount failed. Retrying\n");
						goto umount;
					}
					else
					{
                                		log_err("Unmounting %s failed\n", mount);
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
		
	        /* activate mounts after sleeping 1s to be sure enumeration happened and autoplay will work */
		sleep(1);
                for(i=0 ; mounts[i] != NULL; i++)
                {       
			sprintf(command2, "echo %i  > /sys/devices/platform/musb_hdrc/gadget/gadget-lun%d/nofua", fua, i);
                        log_debug("usb lun = %s active\n", command2);
                        system(command2);
                	sprintf(command2, "echo %s  > /sys/devices/platform/musb_hdrc/gadget/gadget-lun%d/file", mounts[i], i);
                        log_debug("usb lun = %s active\n", command2);
                        system(command2);
                                       }
               g_strfreev(mounts);
	}

	/* only send data in use signal in case we actually succeed */
        if(!ret)
                usb_moded_send_signal(DATA_IN_USE);
	
	return(ret);

}

#ifdef NOKIA
int set_ovi_suite_mode(GList *applist)
{
   int net = 0;

#ifdef APP_SYNC
  /* do not go through the appsync routine if there is no applist */
  if(applist)
  	activate_sync(applist);
  else
        enumerate_usb(NULL);
#else
  system("echo 1 > /sys/devices/platform/musb_hdrc/gadget/softconnect");
#endif /* APP_SYNC */
  /* bring network interface up in case no other network is up */
  net = system("route | grep default");
  if(net)
	  net = system("ifdown usb0 ; ifup usb0");

  /* timeout for exporting CDROM image */
  g_timeout_add_seconds(1, export_cdrom, NULL);

  return(0);
}

gboolean export_cdrom(gpointer data)
{
  const char *path = NULL, *command = NULL;

  path = find_cdrom_path();

  if(path == NULL)
  {
	log_debug("No cdrom path specified => not exporting.\n");
  }
  if(access(path, F_OK) == 0)
  {
  	command = g_strconcat("echo ", path, " > /sys/devices/platform/musb_hdrc/gadget/gadget-lun%d/file", NULL);
	system(command);
  }
  else
	log_debug("Cdrom image file does not exist => no export.\n");

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
        gchar *command;
        char command2[256];
        const char *mount;
        gchar **mounts;
        int ret = 0, i = 0;


        if(!strcmp(module, MODULE_MASS_STORAGE))
        {
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
                                                mount = find_alt_mount();
                                                if(mount)
                                                {
                                                        command = g_strconcat("mount | grep ", mount, NULL);
                                                        ret = system(command);
                                                        g_free(command);
                                                        if(ret)
                                                        {
                                                                command = g_strconcat("mount -t tmpfs tmpfs -o ro -size=512K ", mount, NULL);
                                                                ret = system(command);
                                                                g_free(command);
                                                        }

                                                }

                                        }
                                }

				sprintf(command2, "echo 0  > /sys/devices/platform/musb_hdrc/gadget/gadget-lun%d/file", i);
                		log_debug("usb lun = %s inactive\n", command2);
                		system(command2);
                        }
                        g_strfreev(mounts);
                }

        }
#ifdef NOKIA
        if(!strcmp(module, MODULE_NETWORK))
        {
                /* preventive sync in case of bad quality mtp clients */
                sync();
                /* bring network down immediately */
                system("ifdown usb0");
                /* do soft disconnect */
                system("echo 0 > /sys/devices/platform/musb_hdrc/gadget/softconnect");
		/* DIRTY WORKAROUND: acm/phonet does not work as it should, remove when it does */
		system("killall -SIGTERM acm");
        }
#endif /* NOKIA */


        return(ret);
}

