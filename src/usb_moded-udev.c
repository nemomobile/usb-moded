#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <unistd.h>

#include <poll.h>

#include <libudev.h>

#include <glib.h>

#include "usb_moded-log.h"
#include "usb_moded-config.h"
#include "usb_moded-hw-ab.h"
#include "usb_moded.h"

/* global variables */
static struct udev *udev;
static struct udev_monitor *mon;
static GIOChannel *iochannel;
static guint watch_id; 

/* static function definitions */
static gboolean monitor_udev(GIOChannel *iochannel G_GNUC_UNUSED, GIOCondition cond,
                             gpointer data G_GNUC_UNUSED);
static void udev_parse(struct udev_device *dev);

gboolean hwal_init(void)
{
  const gchar *udev_path = NULL;
  struct udev_device *dev;
  int ret = 0;
	
  /* Create the udev object */
  udev = udev_new();
  if (!udev) 
  {
    log_err("Can't create udev\n");
    return FALSE;
  }
  
  udev_path = find_udev_path();
  if(udev_path)
	dev = udev_device_new_from_syspath(udev, udev_path);
  else
  	dev = udev_device_new_from_syspath(udev, "/sys/class/power_supply/usb");
  if (!dev) 
  {
    log_err("Unable to find $power_supply device.");
    /* communicate failure, mainloop will exit and call appropriate clean-up */
    return FALSE;
  }
  mon = udev_monitor_new_from_netlink (udev, "udev");
  if (!mon) 
  {
    log_err("Unable to monitor the 'present' value\n");
    /* communicate failure, mainloop will exit and call appropriate clean-up */
    return FALSE;
  }
  ret = udev_monitor_filter_add_match_subsystem_devtype(mon, "power_supply", NULL);
  if(ret =! 0)
	return FALSE;

  ret = udev_monitor_enable_receiving (mon);
  if(ret =! 0)
	return FALSE;

  /* check if we are already connected */
  udev_parse(dev);
  
  iochannel = g_io_channel_unix_new(udev_monitor_get_fd(mon));
  watch_id = g_io_add_watch(iochannel, G_IO_IN, monitor_udev, NULL);

  /* everything went well */
  return TRUE;
}

static gboolean monitor_udev(GIOChannel *iochannel G_GNUC_UNUSED, GIOCondition cond,
                             gpointer data G_GNUC_UNUSED)
{
  struct udev_device *dev;

  if(cond & G_IO_IN)
  {
    /* This normally blocks but G_IO_IN indicates that we can read */
    dev = udev_monitor_receive_device (mon);
    if (dev) 
    {
      if(!strcmp(udev_device_get_action(dev), "change"))
      {
	udev_parse(dev);
      }
    }
    /* if we get something else something bad happened stop watching to avoid busylooping */  
    else
	exit(1);
  }
  
  /* keep watching */
  return TRUE;
}

void hwal_cleanup(void)
{
  g_source_remove(watch_id);
  watch_id = 0;
  g_io_channel_unref(iochannel);
  iochannel = NULL;
  udev_monitor_unref(mon);
  udev_unref(udev);
}

static void udev_parse(struct udev_device *dev)
{
  const char *tmp;

  tmp = udev_device_get_property_value(dev, "POWER_SUPPLY_ONLINE");
  if(!tmp)
    tmp = udev_device_get_property_value(dev, "POWER_SUPPLY_PRESENT");
  if(!tmp)
    {
      log_err("No usable power supply indicator\n");
      exit(1);
      // SP: exit? really?
    }
  if(!strcmp(tmp, "1"))
  {
    log_debug("UDEV:power supply present\n");
    /* power supply type might not exist */
    tmp = udev_device_get_property_value(dev, "POWER_SUPPLY_TYPE");
    if(!tmp)
    {
      /* power supply type might not exist also :( Send connected event but this will not be able
      to discriminate between charger/cable */
      log_warning("Fallback since cable detecion cannot be accurate. Will connect on any voltage on usb.\n");
      set_usb_connected(TRUE);
      goto END;
    }
    if(!strcmp(tmp, "USB")||!strcmp(tmp, "USB_CDP"))
    {
      log_debug("UDEV:USB cable connected\n");
      set_usb_connected(TRUE);
    }
  }
  else
  {
    log_debug("UDEV:USB cable disconnected\n");
    set_usb_connected(FALSE);
  }
END:
  udev_device_unref(dev);
  // SP: IMHO: either move the unref to caller or rename the function
}
