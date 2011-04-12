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
struct udev *udev;
struct udev_monitor *mon;
struct udev_device *dev;

/* static function definitions */
gpointer monitor_udev(gpointer data) __attribute__ ((noreturn));
static void udev_parse(struct udev_device *dev);

gboolean hwal_init(void)
{
  GThread * thread;
  const gchar *udev_path = NULL;
	
  /* Create the udev object */
  udev = udev_new();
  if (!udev) 
  {
    log_err("Can't create udev\n");
    return 0;
  }
  
  udev_path = find_udev_path();
  if(udev_path)
	dev = udev_device_new_from_syspath(udev, udev_path);
  else
  	dev = udev_device_new_from_syspath(udev, "/sys/class/power_supply/usb");
  if (!dev) 
  {
    log_err("Unable to find $power_supply device.");
    return 0;
  }
  mon = udev_monitor_new_from_netlink (udev, "udev");
  if (!mon) 
  {
    log_err("Unable to monitor the 'present' value\n");
    return 0;
  }
  udev_monitor_filter_add_match_subsystem_devtype(mon, "power_supply", NULL);
  udev_monitor_enable_receiving (mon);

  /* check if we are already connected */
  udev_parse(dev);
  
  thread = g_thread_create(monitor_udev, NULL, FALSE, NULL);

  if(thread)
  	return 1;
  else
  {
	log_debug("thread not created succesfully\n");
	return 0;
  }
}

gpointer monitor_udev(gpointer data)
{
  while(1)
  {
    dev = udev_monitor_receive_device (mon);
    if (dev) 
    {
      if(!strcmp(udev_device_get_action(dev), "change"))
      {
	udev_parse(dev);
      }
    }
  }
}

void hwal_cleanup(void)
{
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
}
