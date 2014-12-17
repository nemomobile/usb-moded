/**
  @file udev-search.c

  This is a basic utility to check for potential
  viable paths to use for usb_moded.

  This is in case usb_moded can not figure it out for itself.

  compile with gcc -o udev-search udev-search.c -ludev

  Copyright (C) 2014 Jolla. All rights reserved.

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


#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <unistd.h>

#include <poll.h>

#include <libudev.h>


void main (void)
{
  struct udev *udev;
  struct udev_enumerate *list;
  struct udev_list_entry *list_entry, *first_entry;
  const char *udev_name;
  int ret = 0;

  udev = udev_new();
  list = udev_enumerate_new(udev);
  ret  = udev_enumerate_add_match_subsystem(list, "power_supply");
  ret = udev_enumerate_scan_devices(list);
  if(ret < 0)
  {
	printf("List empty\n");
	exit(1);
  }
  first_entry = udev_enumerate_get_list_entry(list);
  udev_list_entry_foreach(list_entry, first_entry)
  {
    udev_name =  udev_list_entry_get_name(list_entry);
    printf("power_supply device name = %s\n", udev_name);
  }
  exit(0);
}
