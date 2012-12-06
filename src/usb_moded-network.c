/*
  @file usb-moded_network.c : (De)activates network depending on the network setting system.

  Copyright (C) 2011 Nokia Corporation. All rights reserved.
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

/*============================================================================= */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "usb_moded-network.h"
#include "usb_moded-config.h"

#if CONNMAN
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>


#endif

/**
 * Activate the network interface
 *
 */
int usb_network_up(void)
{
   const char *ip, *interface, *gateway;
   char command[128];

#if CONNMAN
  DBusConnection *dbus_conn_connman = NULL;
  DBusMessage *msg = NULL, *reply = NULL;
  DBusError error;

  dbus_error_init(&error);

  if( (dbus_conn_connman = dbus_bus_get(DBUS_BUS_SYSTEM, &error)) == 0 )
  {
         log_err("Could not connect to dbus for connman\n");
  }

  if ((msg = dbus_message_new_method_call("net.connman", "/", "net.connman.Service", connect)) != NULL)
  {
        if ((reply = dbus_connection_send_with_reply_and_block(dbus_conn_connman, msg, -1, NULL)) != NULL)
        {
            dbus_message_get_args(reply, NULL, DBUS_TYPE_INT32, &ret, DBUS_TYPE_INVALID);
            dbus_message_unref(reply);
        }
        dbus_message_unref(msg);
  }
  dbus_connection_unref(dbus_conn_connman);

  log_debug("connman state = %d\n", ret);
  return(ret);

#else
  ip = get_network_ip();
  interface = get_network_interface();
  gateway = get_network_gateway();
  if(ip == NULL)
  {
  	system("ifconfig usb0 192.168.2.15");
	return(0);
  }
  if(!strcmp(ip, "dhcp"))
  {
  	if(interface == NULL)
		sprintf(command, "dhclient -d usb0\n");
	else
		sprintf(command, "dhclient -d %s\n", interface);
  }
  if(interface == NULL)
	sprintf(command, "ifconfig usb0 %s\n", ip);	
  else
	sprintf(command, "ifconfig %s %s\n", interface, ip);
  system(command);

  /* TODO: Check first if there is a gateway set */
  if(gateway)
  {
	sprintf(command, "route add default gw %s\n", gateway);
        system(command);
  }

  return(0);
#endif /* CONNMAN */
}

/**
 * Deactivate the network interface
 *
 */
int usb_network_down(void)
{
#if CONNMAN
#else
  system("ifconfig usb0 down");
  return(0);
#endif /* CONNMAN */
}
