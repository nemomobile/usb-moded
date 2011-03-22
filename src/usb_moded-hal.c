/**
  @file usb_moded-hal.c

  Copyright (C) 2010 Nokia Corporation. All rights reserved.

  @author: Philippe De Swert <philippe.de-swert@nokia.com>
  	  Simo Piiroinen <simo.piiroinen@nokia.com>

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

#include <sys/time.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#include <libhal.h>
#include <dbus/dbus-glib-lowlevel.h>

#include "usb_moded.h"
#include "usb_moded-hw-ab.h"
#include "usb_moded-log.h"

/* ========================================================================= *
 * Quick guide to the state logic ...
 *
 * startup:
 * @ hwal_init()
 * - connect to system bus
 * - partially initialize libhal context
 * - listen to hald service ownership changes
 * - check if hald is already running
 *
 * hald running / started:
 * @ hwal_set_hald_running(true)
 * - finalize libhal context
 * - listen to hal device added/removed notifications
 * - check availability of musb device
 *
 * musb device added / available:
 * @ hwal_set_musb_tracking(true)
 * - listen to hal property changes for musb device
 * - broadcast initial state of usb connection
 *
 * musb connection status change:
 * @ hwal_set_usb_connected()
 * - broadcast connection status over dbus
 *
 * musb device removed:
 * @ hwal_set_musb_tracking(false)
 * - stop listening to hal property changes for musb device
 *
 * hald stopped:
 * @ hwal_set_hald_running(false)
 * - stop listening to hal property changes for musb device
 * - shutdown libhal context
 *
 * shutdown:
 * @ hwal_cleanup()
 * - shutdown and free libhal context
 * - forget dbus connection
 *
 * dbus disconnect:
 * - exit
 * ========================================================================= */

/* ========================================================================= *
 * CONSTANTS
 * ========================================================================= */

// FIXME: is there "official" macro for "org.freedesktop.Hal"?
#define HALD_DBUS_SERVICE "org.freedesktop.Hal"

// D-Bus signal match for tracking hal service ownership on system bus
#define MATCH_HALD_OWNERCHANGED\
    "type='signal'"\
    ",sender='"DBUS_SERVICE_DBUS"'"\
    ",interface='"DBUS_INTERFACE_DBUS"'"\
    ",path='"DBUS_PATH_DBUS"'"\
    ",member='NameOwnerChanged'"\
    ",arg0='"HALD_DBUS_SERVICE"'"

// Hal device UDI and property name we are tracking
static const char musb_udi[] = "/org/freedesktop/Hal/devices/platform_musb_hdrc";
static const char musb_prp[] = "button.state.value";

/* ========================================================================= *
 * STATE VARIABLES
 * ========================================================================= */

static DBusConnection *hwal_con  = 0; // system bus connection
static LibHalContext  *hwal_ctx  = 0; // libhal context

static gboolean hald_running  = FALSE; /* Is hald detected on system bus.
					* Set via hwal_set_hald_running() */

static gboolean ctx_attached  = FALSE; /* Is libhal context ready for use.
					* Set via hwal_set_ctx_state() */

static gboolean tracking_musb = FALSE; /* is musb hal device being tracked.
					* Set via hwal_set_musb_tracking() */

/* ========================================================================= *
 * DEBUG UTILITIES
 * ========================================================================= */


/* ------------------------------------------------------------------------- *
 * debug_show_property  --  query & show current state of hal property
 * ------------------------------------------------------------------------- */

#if LOG_ENABLE_DEBUG
static void debug_show_property(const char *udi, const char *key)
{
  DBusError err = DBUS_ERROR_INIT;

  LibHalPropertyType tid;

  tid = libhal_device_get_property_type(hwal_ctx, udi, key, &err);

  if( dbus_error_is_set(&err) )
  {
    log_err("%s: %s: %s\n", __FUNCTION__, err.name, err.message);
    dbus_error_free(&err);
  }

  log_debugf("\t%s = ", key);
  switch( tid )
  {
  case LIBHAL_PROPERTY_TYPE_INVALID:
    log_debugf("invalid");
    break;

  case LIBHAL_PROPERTY_TYPE_INT32:
    log_debugf("int32:%ld", (long)libhal_device_get_property_int(hwal_ctx, udi, key, &err));
    break;

  case LIBHAL_PROPERTY_TYPE_UINT64:
    log_debugf("uint64:%llu", (unsigned long long)libhal_device_get_property_uint64(hwal_ctx, udi, key, &err));
    break;

  case LIBHAL_PROPERTY_TYPE_DOUBLE:
    log_debugf("double:%g", libhal_device_get_property_double(hwal_ctx, udi, key, &err));
    break;

  case LIBHAL_PROPERTY_TYPE_BOOLEAN:
    log_debugf("bool:%s", libhal_device_get_property_bool(hwal_ctx, udi, key, &err) ? "true" : "false");
    break;

  case LIBHAL_PROPERTY_TYPE_STRING:
    log_debugf("string:\"%s\"", libhal_device_get_property_string(hwal_ctx, udi, key, &err));
    break;

  case LIBHAL_PROPERTY_TYPE_STRLIST:
    log_debugf("strlist:{");
    {
      char **v = libhal_device_get_property_strlist(hwal_ctx, udi, key, &err);

      if( v != 0 )
      {
	for( int i = 0; v[i]; ++i ) log_debugf(" \"%s\"", v[i]);
	libhal_free_string_array(v);
      }
    }
    log_debugf(" }");
    break;
  }
  log_debugf("\n");

  if( dbus_error_is_set(&err) )
  {
    log_err("%s: %s: %s\n", __FUNCTION__, err.name, err.message);
    dbus_error_free(&err);
  }
}
#endif // LOG_ENABLE_DEBUG

/* ------------------------------------------------------------------------- *
 * debug_show_property_set  --  show all values in property set
 * ------------------------------------------------------------------------- */

#if LOG_ENABLE_DEBUG
static void debug_show_property_set(LibHalPropertySet *ps)
{
  LibHalPropertySetIterator psi;

  libhal_psi_init(&psi, ps);

  for( ; libhal_psi_has_more(&psi); libhal_psi_next(&psi) )
  {
    log_debugf("\t%s = ", libhal_psi_get_key(&psi));

    switch( libhal_psi_get_type(&psi) )
    {
    case LIBHAL_PROPERTY_TYPE_INVALID:
      log_debugf("invalid");
      break;

    case LIBHAL_PROPERTY_TYPE_INT32:
      log_debugf("int32:%ld", (long)libhal_psi_get_int(&psi));
      break;

    case LIBHAL_PROPERTY_TYPE_UINT64:
      log_debugf("uint64:%llu", (unsigned long long)libhal_psi_get_uint64(&psi));
      break;

    case LIBHAL_PROPERTY_TYPE_DOUBLE:
      log_debugf("double:%g", libhal_psi_get_double(&psi));
      break;

    case LIBHAL_PROPERTY_TYPE_BOOLEAN:
      log_debugf("bool:%s", libhal_psi_get_bool(&psi) ? "true" : "false");
      break;

    case LIBHAL_PROPERTY_TYPE_STRING:
      log_debugf("string:\"%s\"", libhal_psi_get_string(&psi));
      break;

    case LIBHAL_PROPERTY_TYPE_STRLIST:
      log_debugf("strlist:{");
      {
	char **v = libhal_psi_get_strlist(&psi);
	if( v ) for( int i = 0; v[i]; ++i ) log_debugf(" \"%s\"", v[i]);
      }
      log_debugf(" }");
      break;
    }
    log_debugf("\n");
  }
}
#endif // LOG_ENABLE_DEBUG

/* ========================================================================= *
 * LIBHAL UTILITIES
 * ========================================================================= */

/* ------------------------------------------------------------------------- *
 * halipc_get_all_properties
 * ------------------------------------------------------------------------- */

static LibHalPropertySet *halipc_get_all_properties(const char *udi)
{
  DBusError err = DBUS_ERROR_INIT;
  LibHalPropertySet *res = 0;

  res = libhal_device_get_all_properties(hwal_ctx, udi, &err);

  if( dbus_error_is_set(&err) )
  {
    log_err("%s: %s: %s\n", __FUNCTION__, err.name, err.message);
    dbus_error_free(&err);
  }

  return res;
}

/* ------------------------------------------------------------------------- *
 * halipc_device_exists
 * ------------------------------------------------------------------------- */

static int halipc_device_exists(const char *udi)
{
  DBusError err = DBUS_ERROR_INIT;
  int       res = libhal_device_exists(hwal_ctx, udi, &err);

  if( dbus_error_is_set(&err) )
  {
    log_err("%s: %s: %s\n", __FUNCTION__, err.name, err.message);
    dbus_error_free(&err);
  }

  return res;
}

/* ------------------------------------------------------------------------- *
 * halipc_device_add_watch
 * ------------------------------------------------------------------------- */

static int halipc_device_add_watch(const char *udi)
{
  DBusError err = DBUS_ERROR_INIT;
  int       res = libhal_device_add_property_watch(hwal_ctx, udi, &err);

  if( dbus_error_is_set(&err) )
  {
    log_err("%s: %s: %s\n", __FUNCTION__, err.name, err.message);
    dbus_error_free(&err);
  }

  return res;
}

/* ------------------------------------------------------------------------- *
 * halipc_device_remove_watch
 * ------------------------------------------------------------------------- */

static int halipc_device_remove_watch(const char *udi)
{
  DBusError err = DBUS_ERROR_INIT;
  int       res = libhal_device_remove_property_watch(hwal_ctx, udi, &err);

  if( dbus_error_is_set(&err) )
  {
    log_err("%s: %s: %s\n", __FUNCTION__, err.name, err.message);
    dbus_error_free(&err);
  }

  return res;
}

/* ------------------------------------------------------------------------- *
 * halipc_type_to_string
 * ------------------------------------------------------------------------- */

static const char *halipc_type_to_string(LibHalPropertyType tid)
{
  switch( tid )
  {
  case LIBHAL_PROPERTY_TYPE_INVALID: return "invalid";
  case LIBHAL_PROPERTY_TYPE_INT32:   return "int32";
  case LIBHAL_PROPERTY_TYPE_UINT64:  return "uint64";
  case LIBHAL_PROPERTY_TYPE_DOUBLE:  return "double";
  case LIBHAL_PROPERTY_TYPE_BOOLEAN: return "boolean";
  case LIBHAL_PROPERTY_TYPE_STRING:  return "string";
  case LIBHAL_PROPERTY_TYPE_STRLIST: return "strlist";
  }
  return "unknown";
}

/* ========================================================================= *
 * STATE LOGIC FUNCTIONS
 * ========================================================================= */

/* ------------------------------------------------------------------------- *
 * hwal_set_usb_connected  --  forward usb connection status
 * ------------------------------------------------------------------------- */

static void hwal_set_usb_connected(gboolean connected)
{
  log_notice("%s(%s)\n", __FUNCTION__, connected ? "CONNECTED" : "DISCONNECTED");
  set_usb_connected(connected);
}

/* ------------------------------------------------------------------------- *
 * hwal_set_musb_tracking  --  start/stop usb device property tracking
 * ------------------------------------------------------------------------- */

static void hwal_set_musb_tracking(int enable)
{
  if( enable )
  {
    if( ctx_attached && !tracking_musb )
    {
      if( (tracking_musb = halipc_device_add_watch(musb_udi)) )
      {
	LibHalPropertySet *ps = 0;
	LibHalPropertyType tid;

	if( (ps = halipc_get_all_properties(musb_udi)) )
	{
#if LOG_ENABLE_DEBUG
	  if( log_get_level() >= LOG_DEBUG ) debug_show_property_set(ps);
#endif
	  tid = libhal_ps_get_type(ps, musb_prp);

	  if( tid != LIBHAL_PROPERTY_TYPE_BOOLEAN )
	  {
	    log_err("%s: %s: expected '%s', got '%s'\n",
		      musb_udi, musb_prp,
		      halipc_type_to_string(LIBHAL_PROPERTY_TYPE_BOOLEAN),
		      halipc_type_to_string(tid));
	  }
	  else
	  {
	    hwal_set_usb_connected(libhal_ps_get_bool(ps, musb_prp));
	  }

	  libhal_free_property_set(ps);
	}
      }
    }
  }
  else
  {
    if( tracking_musb )
    {
      halipc_device_remove_watch(musb_udi);
      tracking_musb = 0;
    }
  }
  log_notice("%s(%s) -> %s\n", __FUNCTION__,
	     enable        ? "ENABLE"  : "DISABLE",
	     tracking_musb ? "ENABLED" : "DISABLED");
}

/* ------------------------------------------------------------------------- *
 * hwal_set_ctx_state
 * ------------------------------------------------------------------------- */

static gboolean hwal_set_ctx_state(gboolean attach)
{
  if( attach )
  {
    if( hwal_ctx != 0 && !ctx_attached )
    {
      DBusError err = DBUS_ERROR_INIT;

      if( !libhal_ctx_init(hwal_ctx, &err) )
      {
	log_err("%s: %s: %s\n", "ctx init",  err.name, err.message);
	dbus_error_free(&err);
      }
      else
      {
	ctx_attached = TRUE;
      }
    }
  }
  else
  {
    if( hwal_ctx != 0 && ctx_attached )
    {
      DBusError err = DBUS_ERROR_INIT;

      if( !libhal_ctx_shutdown(hwal_ctx, &err) )
      {
	log_err("%s: %s: %s\n", "ctx shutdown",  err.name, err.message);
	dbus_error_free(&err);
      }

      ctx_attached = FALSE;
    }
  }

  log_notice("%s(%s) -> %s\n", __FUNCTION__,
	     attach       ? "ATTACH"   : "DETACH",
	     ctx_attached ? "ATTACHED" : "DETACHED");

  return ctx_attached;
}

/* ------------------------------------------------------------------------- *
 * hwal_set_hald_running  --  react to hal run state changes
 * ------------------------------------------------------------------------- */

static void hwal_set_hald_running(gboolean running)
{
  hald_running = running;

  log_notice("HALD -> %s\n", hald_running ? "RUNNING" : "STOPPED");

  if( hald_running )
  {
    hwal_set_ctx_state(TRUE);
    hwal_set_musb_tracking(halipc_device_exists(musb_udi));
  }
  else
  {
    hwal_set_musb_tracking(FALSE);
    hwal_set_ctx_state(FALSE);
  }
}

/* ========================================================================= *
 * CALLBACK FUNCTIONS
 * ========================================================================= */

/* ------------------------------------------------------------------------- *
 * hwal_property_modified_cb  --  handle hal property modified notification
 * ------------------------------------------------------------------------- */

static void
hwal_property_modified_cb(LibHalContext *ctx, const char *udi, const char *key,
			  dbus_bool_t is_removed, dbus_bool_t is_added)
{
  log_info("%s(%s, %s, %d, %d)\n", __FUNCTION__, udi, key, is_removed, is_added);

  if( udi && !strcmp(udi, musb_udi) )
  {
#if LOG_ENABLE_DEBUG
    // usb device prop changed -> debug output
    if( !is_removed )
    {
      if( log_get_level() >= LOG_DEBUG ) debug_show_property(udi, key);
    }
#endif

    if( key && !strcmp(key, musb_prp) )
    {
      // state property changed -> propagate change
      DBusError   err = DBUS_ERROR_INIT;
      dbus_bool_t val = libhal_device_get_property_bool(ctx, udi, key, &err);

      if( dbus_error_is_set(&err) )
      {
	log_err("%s: %s: %s\n", "get property bool", err.name, err.message);
	dbus_error_free(&err);
      }
      else
      {
	hwal_set_usb_connected(val);
      }
    }
  }
}

/* ------------------------------------------------------------------------- *
 * hwal_device_added_cb  --  handle hal device added notification
 * ------------------------------------------------------------------------- */

static void
hwal_device_added_cb(LibHalContext *ctx, const char *udi)
{
  log_info("%s(%s)\n", __FUNCTION__, udi);

  if( udi && !strcmp(udi, musb_udi) )
  {
    // usb device was added -> track properties if hald is running
    hwal_set_musb_tracking(hald_running);
  }
}

/* ------------------------------------------------------------------------- *
 * hwal_device_removed_cb  -- handle hal device removed notification
 * ------------------------------------------------------------------------- */

static void
hwal_device_removed_cb(LibHalContext *ctx, const char *udi)
{
  log_info("%s(%s)\n", __FUNCTION__, udi);

  if( udi && !strcmp(udi, musb_udi) )
  {
    // usb device was removed ->  stop tracking properties
    hwal_set_musb_tracking(FALSE);
  }
}

/* ------------------------------------------------------------------------- *
 * hwal_dbus_message_cb  -- handle dbus message
 * ------------------------------------------------------------------------- */

static
DBusHandlerResult
hwal_dbus_message_cb(DBusConnection *conn,
		     DBusMessage *msg,
		     void *user_data)
{
  DBusHandlerResult   result    = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  const char         *interface = dbus_message_get_interface(msg);
  const char         *member    = dbus_message_get_member(msg);
  const char         *object    = dbus_message_get_path(msg);
  int                 type      = dbus_message_get_type(msg);
  const char         *typestr   = dbus_message_type_to_string(type);
  DBusError   err  = DBUS_ERROR_INIT;

  if( !interface || !member || !object )
  {
    goto cleanup;
  }

  log_debug("DBUS: %s - %s.%s(%s)\n", typestr, interface, member, object);

  if( type == DBUS_MESSAGE_TYPE_SIGNAL )
  {
    if( !strcmp(interface, DBUS_INTERFACE_DBUS) && !strcmp(object,DBUS_PATH_DBUS) )
    {
      if( !strcmp(member, "NameOwnerChanged") )
      {
	const char *prev = 0;
	const char *curr = 0;
	const char *serv = 0;

	if (! dbus_message_get_args(msg, &err,
			      DBUS_TYPE_STRING, &serv,
			      DBUS_TYPE_STRING, &prev,
			      DBUS_TYPE_STRING, &curr,
			      DBUS_TYPE_INVALID) )
	{
		goto cleanup;
	}
	else
	{
		log_debug("service: \"%s\", old: \"%s\", new: \"%s\"\n", serv, prev, curr);
	}
	if( serv != 0 && !strcmp(serv, HALD_DBUS_SERVICE) )
	{
	  hwal_set_hald_running(curr && *curr);
	}

      }
    }
  }

cleanup:

  dbus_error_free(&err);
  return result;
}

/* ------------------------------------------------------------------------- *
 * hwal_init  --  set up the hw abstraction layer
 * ------------------------------------------------------------------------- */

gboolean hwal_init(void)
{
  gboolean    res = FALSE;
  DBusError   err = DBUS_ERROR_INIT;

  // initialize system bus connection

  if( (hwal_con = dbus_bus_get(DBUS_BUS_SYSTEM, &err)) == 0 )
  {
    log_crit("%s: %s: %s\n", "bus get", err.name, err.message);
    goto cleanup;
  }

  if( !dbus_connection_add_filter(hwal_con, hwal_dbus_message_cb, 0, 0) )
  {
    goto cleanup;
  }

  dbus_bus_add_match(hwal_con, MATCH_HALD_OWNERCHANGED, &err);
  if( dbus_error_is_set(&err) )
  {
    log_crit("%s: %s: %s\n", "add match", err.name, err.message);
    goto cleanup;
  }

  dbus_connection_set_exit_on_disconnect(hwal_con, TRUE);

  dbus_connection_setup_with_g_main(hwal_con, NULL);

  // partially initialize libhal context

  if( (hwal_ctx = libhal_ctx_new()) == 0 )
  {
    goto cleanup;
  }

  libhal_ctx_set_dbus_connection(hwal_ctx, hwal_con);

  libhal_ctx_set_device_added(hwal_ctx, hwal_device_added_cb);
  libhal_ctx_set_device_removed(hwal_ctx, hwal_device_removed_cb);
  libhal_ctx_set_device_property_modified(hwal_ctx, hwal_property_modified_cb);

  // check if hald is already present on system bus

  if( !dbus_bus_name_has_owner(hwal_con, HALD_DBUS_SERVICE, &err) )
  {
    if( dbus_error_is_set(&err) )
    {
      log_err("%s: %s: %s\n", "has owner", err.name, err.message);
      dbus_error_free(&err);
    }
    hwal_set_hald_running(FALSE);
  }
  else
  {
    hwal_set_hald_running(TRUE);
  }

  // success

  res = TRUE;

cleanup:

  dbus_error_free(&err);
  return res;
}

/* ------------------------------------------------------------------------- *
 * hwal_cleanup  --  cleans up the hw abstraction layer on exit
 * ------------------------------------------------------------------------- */

void hwal_cleanup(void)
{
  // drop libhal context
  if( hwal_ctx  != 0 )
  {
    hwal_set_ctx_state(FALSE);
    libhal_ctx_free(hwal_ctx);
    hwal_ctx = 0;
  }

  // drop system bus connection
  if( hwal_con != 0 )
  {
    DBusError err = DBUS_ERROR_INIT;

    // stop listening to signals
    dbus_bus_remove_match(hwal_con, MATCH_HALD_OWNERCHANGED, &err);
    if( dbus_error_is_set(&err) )
    {
      log_err("%s: %s: %s\n", "remove match", err.name, err.message);
      dbus_error_free(&err);
    }

    // remove message handlers
    dbus_connection_remove_filter(hwal_con, hwal_dbus_message_cb, 0);

    // forget the connection
    dbus_connection_unref(hwal_con);
    hwal_con = 0;

    // TODO: detach from the mainloop, how?
  }
}
