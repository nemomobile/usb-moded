#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <unistd.h>

#include <poll.h>

#include <libudev.h>

#include "usb_moded-loh.h"

/* global variables */
struct udev *udev;
struct udev_monitor *mon;
struct udev_device *dev;

int hwal_init(void)
{
  struct udev_list_entry *devices, *dev_list_entry;
  struct pollfd *fds;
  int poll_ret = 0;
	
  /* Create the udev object */
  udev = udev_new();
  if (!udev) 
  {
    log_err("Can't create udev\n");
    return 1;
  }

  /* Set up a monitor to monitor devices */
  mon = udev_monitor_new_from_netlink(udev, "udev");
  udev_monitor_filter_add_match_subsystem_devtype(mon, "usb", NULL);
  udev_monitor_enable_receiving(mon);
  /* Get the file descriptor (fd) for the monitor.
  This fd will get passed to select() */
  fd = udev_monitor_get_fd(mon);

  /* loop waiting for events */
  while(1)
  {
    /* set a poll on the fd so we do not block trying to read data */
    fds = malloc(sizeof(struct pollfd));
    fds->fd = fd;
    fds->events = POLLPRI;
    poll_ret = poll(fds, 1, -1);
    if(poll_ret)
    { 
      dev = udev_monitor_receive_device(mon);
      if(dev) 
      {
	log_debug("Got Device\n");
	log_debug("   Node: %s\n", udev_device_get_devnode(dev));
	log_debug("   Subsystem: %s\n", udev_device_get_subsystem(dev));
	log_debug("   Devtype: %s\n", udev_device_get_devtype(dev));
	log_debug("   Action: %s\n", udev_device_get_action(dev));
        udev_device_unref(dev);
      }

    }
 
}

int hwal_cleanup(void)
{
  udev_enumerate_unref(enumerate);
  udev_unref(udev);
  return 0;       
}


