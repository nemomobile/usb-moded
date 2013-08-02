/**
  @file usb_moded-android.c

  Copyright (C) 2013 Jolla. All rights reserved.

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
#include <glib.h>

#include "usb_moded-android.h"
#include "usb_moded-log.h"
#include "usb_moded-modesetting.h"
#include "usb_moded-config.h"

/** check if android settings are set
 *
 * @return 1 if settings are available, 0 if not
 *
 */
int android_settings(void)
{
  int ret = 0;
  
  ret = check_android_section();

  return ret;
}

/** initialize the basic android values
 */
void android_init_values(void)
{
  const char *text;

  text = get_android_manufacturer();
  if(text)
  {
	write_to_file("/sys/class/android_usb/android0/iManufacturer", text);
	g_free((char *)text);
  }
  text = get_android_vendor();
  if(text)
  {
	write_to_file("/sys/class/android_usb/android0/idVendor", text);
	g_free((char *)text);
  }
  text = get_android_product();
  if(text)
  {
	write_to_file("/sys/class/android_usb/android0/iProduct", text);
	g_free((char *)text);
  }
  
}
