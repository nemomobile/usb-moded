/*
 
  Copyright (C) 2010 Nokia Corporation. All rights reserved.

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


#define FS_MOUNT_CONFIG_FILE		"/etc/usb-moded/usb-moded.ini"

#define FS_MOUNT_DEFAULT	 	"/dev/mmcblk0p1"
#define FS_MOUNT_ENTRY			"mountpoints"
#define FS_MOUNT_KEY			"mount"
#define FS_SYNC_ENTRY			"sync"
#define FS_SYNC_KEY			"nofua"
#define ALT_MOUNT_ENTRY			"altmount"
#define ALT_MOUNT_KEY			"mount"
#define UDEV_PATH_ENTRY			"udev"
#define UDEV_PATH_KEY			"path"
#define CDROM_ENTRY			"cdrom"
#define CDROM_PATH_KEY			"path"
#define CDROM_TIMEOUT_KEY		"timeout"
#define TRIGGER_ENTRY			"trigger"
#define TRIGGER_PATH_KEY		"path"
#define TRIGGER_MODE_KEY		"mode"

const char * find_mounts(void);
int find_sync(void);
const char * find_alt_mount(void);

#ifdef UDEV
const char * find_udev_path(void);
#endif

#ifdef NOKIA
const char * find_cdrom_path(void);
int find_cdrom_timeout(void);
#endif

#ifdef APP_SYNC
const char * check_trigger(void);
const char * check_trigger_mode(void);
#endif /* APP_SYNC */
