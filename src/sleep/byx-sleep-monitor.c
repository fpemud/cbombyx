/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * (C) Copyright 2012-2016 Red Hat, Inc.
 * Author: Matthias Clasen <mclasen@redhat.com>
 */

#include "byx-default.h"

#include "nm-sleep-monitor.h"

#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <gio/gunixfdlist.h>

#include "nm-core-internal.h"
#include "NetworkManagerUtils.h"

#if defined (SUSPEND_RESUME_SYSTEMD) || defined (SUSPEND_RESUME_ELOGIND)

#define SUSPEND_DBUS_NAME               "org.freedesktop.login1"
#define SUSPEND_DBUS_PATH               "/org/freedesktop/login1"
#define SUSPEND_DBUS_INTERFACE          "org.freedesktop.login1.Manager"
#if defined (SUSPEND_RESUME_SYSTEMD)
#define _NMLOG_PREFIX_NAME              "sleep-monitor-sd"
#else
#define _NMLOG_PREFIX_NAME              "sleep-monitor-el"
#endif

#else

#error define one of SUSPEND_RESUME_SYSTEMD or SUSPEND_RESUME_ELOGIND

#endif

/*****************************************************************************/

enum {
	SLEEPING,
	LAST_SIGNAL,
};

static guint signals[LAST_SIGNAL] = {0};

struct _ByxSleepMonitor {
	GObject parent;

	GDBusProxy *proxy;

	/* used both during construction of proxy and during Inhibit call. */
	GCancellable *cancellable;

	gint inhibit_fd;
	GSList *handles_active;
	GSList *handles_stale;

	gulong sig_id_1;
	gulong sig_id_2;
};

struct _ByxSleepMonitorClass {
	GObjectClass parent;
};

G_DEFINE_TYPE (ByxSleepMonitor, byx_sleep_monitor, G_TYPE_OBJECT);

/*****************************************************************************/

#define _NMLOG_DOMAIN      LOGD_SUSPEND
#define _NMLOG(level, ...) __NMLOG_DEFAULT (level, _NMLOG_DOMAIN, _NMLOG_PREFIX_NAME, __VA_ARGS__)

/*****************************************************************************/

static void sleep_signal (ByxSleepMonitor *self, gboolean is_about_to_suspend);

/*****************************************************************************/

static void
drop_inhibitor (ByxSleepMonitor *self, gboolean force)
{
	if (!force && self->handles_active)
		return;

	if (self->inhibit_fd >= 0) {
		_LOGD ("inhibit: dropping sleep inhibitor %d", self->inhibit_fd);
		nm_close (self->inhibit_fd);
		self->inhibit_fd = -1;
	}

	if (self->handles_active) {
		self->handles_stale = g_slist_concat (self->handles_stale, self->handles_active);
		self->handles_active = NULL;
	}

	nm_clear_g_cancellable (&self->cancellable);
}

static void
inhibit_done (GObject      *source,
              GAsyncResult *result,
              gpointer      user_data)
{
	GDBusProxy *proxy = G_DBUS_PROXY (source);
	ByxSleepMonitor *self = user_data;
	g_autoptr(GError) error = NULL;
	gs_unref_variant GVariant *res = NULL;
	gs_unref_object GUnixFDList *fd_list = NULL;

	res = g_dbus_proxy_call_with_unix_fd_list_finish (proxy, &fd_list, result, &error);
	if (!res) {
		if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
			g_clear_object (&self->cancellable);
			_LOGW ("inhibit: failed (%s)", error->message);
		}
		return;
	}

	g_clear_object (&self->cancellable);

	if (!fd_list || g_unix_fd_list_get_length (fd_list) != 1) {
		_LOGW ("inhibit: didn't get a single fd back");
		return;
	}

	self->inhibit_fd = g_unix_fd_list_get (fd_list, 0, NULL);
	_LOGD ("inhibit: inhibitor fd is %d", self->inhibit_fd);
}

static void
take_inhibitor (ByxSleepMonitor *self)
{
	g_return_if_fail (BYX_IS_SLEEP_MONITOR (self));
	g_return_if_fail (G_IS_DBUS_PROXY (self->proxy));

	drop_inhibitor (self, TRUE);

	_LOGD ("inhibit: taking sleep inhibitor...");
	self->cancellable = g_cancellable_new ();
	g_dbus_proxy_call_with_unix_fd_list (self->proxy,
	                                     "Inhibit",
	                                     g_variant_new ("(ssss)",
	                                                    "sleep",
	                                                    "NetworkManager",
	                                                    "NetworkManager needs to turn off networks",
	                                                    "delay"),
	                                     0,
	                                     G_MAXINT,
	                                     NULL,
	                                     self->cancellable,
	                                     inhibit_done,
	                                     self);
}

static void
prepare_for_sleep_cb (GDBusProxy  *proxy,
                      gboolean     is_about_to_suspend,
                      gpointer     data)
{
	sleep_signal (data, is_about_to_suspend);
}

static void
name_owner_cb (GObject    *object,
               GParamSpec *pspec,
               gpointer    user_data)
{
	GDBusProxy *proxy = G_DBUS_PROXY (object);
	ByxSleepMonitor *self = BYX_SLEEP_MONITOR (user_data);
	char *owner;

	g_assert (proxy == self->proxy);

	owner = g_dbus_proxy_get_name_owner (proxy);
	if (owner)
		take_inhibitor (self);
	else
		drop_inhibitor (self, TRUE);
	g_free (owner);
}

static void
sleep_signal (ByxSleepMonitor *self,
              gboolean is_about_to_suspend)
{
	g_return_if_fail (BYX_IS_SLEEP_MONITOR (self));

	_LOGD ("received %s signal", is_about_to_suspend ? "SLEEP" : "RESUME");

	if (!is_about_to_suspend)
		take_inhibitor (self);

	g_signal_emit (self, signals[SLEEPING], 0, is_about_to_suspend);

	if (is_about_to_suspend)
		drop_inhibitor (self, FALSE);
}

/**
 * byx_sleep_monitor_inhibit_take:
 * @self: the #ByxSleepMonitor instance
 *
 * Prevent the release of inhibitor lock
 *
 * Returns: an inhibitor handle that must be returned via
 *   byx_sleep_monitor_inhibit_release().
 **/
ByxSleepMonitorInhibitorHandle *
byx_sleep_monitor_inhibit_take (ByxSleepMonitor *self)
{
	g_return_val_if_fail (BYX_IS_SLEEP_MONITOR (self), NULL);

	self->handles_active = g_slist_prepend (self->handles_active, NULL);
	return (ByxSleepMonitorInhibitorHandle *) self->handles_active;
}

/**
 * byx_sleep_monitor_inhibit_release:
 * @self: the #ByxSleepMonitor instance
 * @handle: the #ByxSleepMonitorInhibitorHandle inhibitor handle.
 *
 * Allow again the release of inhibitor lock
 **/
void
byx_sleep_monitor_inhibit_release (ByxSleepMonitor *self,
                                  ByxSleepMonitorInhibitorHandle *handle)
{
	GSList *l;

	g_return_if_fail (BYX_IS_SLEEP_MONITOR (self));
	g_return_if_fail (handle);

	l = (GSList *) handle;

	if (g_slist_position (self->handles_active, l) < 0) {
		if (g_slist_position (self->handles_stale, l) < 0)
			g_return_if_reached ();
		self->handles_stale = g_slist_delete_link (self->handles_stale, l);
		return;
	}

	self->handles_active = g_slist_delete_link (self->handles_active, l);

	drop_inhibitor (self, FALSE);
}

static void
on_proxy_acquired (GObject *object,
                   GAsyncResult *res,
                   ByxSleepMonitor *self)
{
	GError *error = NULL;
	GDBusProxy *proxy;

	proxy = g_dbus_proxy_new_for_bus_finish (res, &error);
	if (!proxy) {
		if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
			_LOGW ("failed to acquire D-Bus proxy: %s", error->message);
		g_clear_error (&error);
		return;
	}
	self->proxy = proxy;
	g_clear_object (&self->cancellable);

	self->sig_id_1 = g_signal_connect (self->proxy, "notify::g-name-owner",
	                                   G_CALLBACK (name_owner_cb), self);
	self->sig_id_2 = _nm_dbus_signal_connect (self->proxy, "PrepareForSleep",
	                                          G_VARIANT_TYPE ("(b)"),
	                                          G_CALLBACK (prepare_for_sleep_cb), self);
	{
		g_autofree char *owner = NULL;

		owner = g_dbus_proxy_get_name_owner (self->proxy);
		if (owner)
			take_inhibitor (self);
	}
}

/*****************************************************************************/

static void
byx_sleep_monitor_init (ByxSleepMonitor *self)
{
	self->inhibit_fd = -1;
	self->cancellable = g_cancellable_new ();
	g_dbus_proxy_new_for_bus (G_BUS_TYPE_SYSTEM,
	                          G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START |
	                          G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES,
	                          NULL,
	                          SUSPEND_DBUS_NAME, SUSPEND_DBUS_PATH, SUSPEND_DBUS_INTERFACE,
	                          self->cancellable,
	                          (GAsyncReadyCallback) on_proxy_acquired, self);
}

ByxSleepMonitor *
byx_sleep_monitor_new (void)
{
	return g_object_new (BYX_TYPE_SLEEP_MONITOR, NULL);
}

static void
dispose (GObject *object)
{
	ByxSleepMonitor *self = BYX_SLEEP_MONITOR (object);

	drop_inhibitor (self, TRUE);

	nm_clear_g_cancellable (&self->cancellable);

	if (self->proxy) {
		nm_clear_g_signal_handler (self->proxy, &self->sig_id_1);
		nm_clear_g_signal_handler (self->proxy, &self->sig_id_2);
		g_clear_object (&self->proxy);
	}

	G_OBJECT_CLASS (byx_sleep_monitor_parent_class)->dispose (object);
}

static void
byx_sleep_monitor_class_init (ByxSleepMonitorClass *klass)
{
	GObjectClass *gobject_class;

	gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->dispose = dispose;

	signals[SLEEPING] = g_signal_new (BYX_SLEEP_MONITOR_SIGNAL_SLEEPING,
	                                  BYX_TYPE_SLEEP_MONITOR,
	                                  G_SIGNAL_RUN_LAST,
	                                  0, NULL, NULL,
	                                  g_cclosure_marshal_VOID__BOOLEAN,
	                                  G_TYPE_NONE, 1, G_TYPE_BOOLEAN);
}

