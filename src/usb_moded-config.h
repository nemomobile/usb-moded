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

#define MODE_SETTING_ENTRY		"usbmode"
#define MODE_SETTING_KEY		"mode"
#define FS_MOUNT_DEFAULT	 	"/dev/mmcblk0p1"
#define FS_MOUNT_ENTRY			"mountpoints"
#define FS_MOUNT_KEY			"mount"
#define FS_SYNC_ENTRY			"sync"
#define FS_SYNC_KEY			"nofua"
#define ALT_MOUNT_ENTRY			"altmount"
#define ALT_MOUNT_KEY			"mount"
#define UDEV_PATH_ENTRY			"udev"
#define UDEV_PATH_KEY			"path"
#define UDEV_SUBSYSTEM_KEY		"subsystem"
#define CDROM_ENTRY			"cdrom"
#define CDROM_PATH_KEY			"path"
#define CDROM_TIMEOUT_KEY		"timeout"
#define TRIGGER_ENTRY			"trigger"
#define TRIGGER_PATH_KEY		"path"
#define TRIGGER_UDEV_SUBSYSTEM		"udev_subsystem"
#define TRIGGER_MODE_KEY		"mode"
#define TRIGGER_PROPERTY_KEY		"property"
#define TRIGGER_PROPERTY_VALUE_KEY	"value"
#define NETWORK_ENTRY			"network"
#define NETWORK_IP_KEY			"ip"
#define NETWORK_INTERFACE_KEY		"interface"
#define NETWORK_GATEWAY_KEY		"gateway"

const char * find_mounts(void);
int find_sync(void);
const char * find_alt_mount(void);

#ifdef UDEV
const char * find_udev_path(void);
const char * find_udev_subsystem(void);
#endif

#ifdef NOKIA
const char * find_cdrom_path(void);
int find_cdrom_timeout(void);
#endif

#ifdef UDEV
const char * check_trigger(void);
const char * get_trigger_subsystem(void);
const char * get_trigger_mode(void);
const char * get_trigger_property(void);
const char * get_trigger_value(void);
#endif /* UDEV */

const char * get_network_ip(void);
const char * get_network_interface(void);
const char * get_network_gateway(void);
