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

#define CONF_DIR_PATH	"/etc/usb-moded/run"

#define APP_INFO_ENTRY		"info"
#define APP_INFO_NAME_KEY	"name"
#define APP_INFO_LAUNCH_KEY	"launch"

typedef struct list_elem
{
  char *name;
  char *launch;
  int active;
}list_elem;

GList *readlist(void);
int activate_sync(GList *list);
int mark_active(GList *list, const gchar *name);
gboolean enumerate_usb(gpointer data);
