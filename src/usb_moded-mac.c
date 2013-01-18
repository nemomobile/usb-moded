/**
  @file usb_moded-mac.c

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
#include "usb_moded-mac.h"
#include "usb_moded-log.h"

static void random_ether_addr(unsigned char *addr)
{
  FILE *random; 

  random = fopen("/dev/urandom", "r");
  fread(addr, 1, 6, random); 
  fclose(random);

  addr [0] &= 0xfe;       /* clear multicast bit */
  addr [0] |= 0x02;       /* set local assignment bit (IEEE802) */
}

void generate_random_mac (void)
{
  unsigned char addr[6];
  int i;
  FILE *g_ether;

  log_debug("Getting random usb ethernet mac\n");
  random_ether_addr(addr);
  
  g_ether = fopen("/etc/modprobe.d/g_ether.conf", "w");
  fprintf(g_ether, "options g_ether host_addr=");
 
  for(i=0; i<5; i++)
  {
	fprintf(g_ether, "%02x:",addr[i]);
  }
  fprintf(g_ether, "%02x\n",addr[i]);
  fclose(g_ether);
}
