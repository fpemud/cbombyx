/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

/* NetworkManager -- Network link manager
 *
 * This program is free software; you can redistribute it and/or modify
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
 * Copyright (C) 2006 - 2008 Red Hat, Inc.
 * Copyright (C) 2006 - 2008 Novell, Inc.
 */

#ifndef __BYX_DBUS_MANAGER_H__
#define __BYX_DBUS_MANAGER_H__

#include "nm-dbus-utils.h"

#define BYX_TYPE_DBUS_MANAGER (byx_dbus_manager_get_type ())
#define BYX_DBUS_MANAGER(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), BYX_TYPE_DBUS_MANAGER, ByxDBusManager))
#define BYX_DBUS_MANAGER_CLASS(k) (G_TYPE_CHECK_CLASS_CAST((k), BYX_TYPE_DBUS_MANAGER, ByxDBusManagerClass))
#define BYX_IS_DBUS_MANAGER(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), BYX_TYPE_DBUS_MANAGER))
#define BYX_IS_DBUS_MANAGER_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), BYX_TYPE_DBUS_MANAGER))
#define BYX_DBUS_MANAGER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BYX_TYPE_DBUS_MANAGER, ByxDBusManagerClass))

#define BYX_DBUS_MANAGER_PRIVATE_CONNECTION_NEW           "private-connection-new"
#define BYX_DBUS_MANAGER_PRIVATE_CONNECTION_DISCONNECTED  "private-connection-disconnected"

typedef struct _ByxDBusManagerClass ByxDBusManagerClass;

GType byx_dbus_manager_get_type (void);

ByxDBusManager *byx_dbus_manager_get (void);

typedef void (*ByxDBusManagerSetPropertyHandler) (ByxDBusObject *obj,
                                                 const ByxDBusInterfaceInfoExtended *interface_info,
                                                 const ByxDBusPropertyInfoExtended *property_info,
                                                 GDBusConnection *connection,
                                                 const char *sender,
                                                 GDBusMethodInvocation *invocation,
                                                 GVariant *value,
                                                 gpointer user_data);

gboolean byx_dbus_manager_acquire_bus (ByxDBusManager *self);

void byx_dbus_manager_start (ByxDBusManager *self,
                            ByxDBusManagerSetPropertyHandler set_property_handler,
                            gpointer set_property_handler_data);

void byx_dbus_manager_stop (ByxDBusManager *self);

gboolean byx_dbus_manager_is_stopping (ByxDBusManager *self);

GDBusConnection *byx_dbus_manager_get_connection (ByxDBusManager *self);

ByxDBusObject *byx_dbus_manager_lookup_object (ByxDBusManager *self, const char *path);

void _byx_dbus_manager_obj_export (ByxDBusObject *obj);
void _byx_dbus_manager_obj_unexport (ByxDBusObject *obj);
void _byx_dbus_manager_obj_notify (ByxDBusObject *obj,
                                  guint n_pspecs,
                                  const GParamSpec *const*pspecs);
void _byx_dbus_manager_obj_emit_signal (ByxDBusObject *obj,
                                       const ByxDBusInterfaceInfoExtended *interface_info,
                                       const GDBusSignalInfo *signal_info,
                                       GVariant *args);

gboolean byx_dbus_manager_get_caller_info (ByxDBusManager *self,
                                          GDBusMethodInvocation *context,
                                          char **out_sender,
                                          gulong *out_uid,
                                          gulong *out_pid);

gboolean byx_dbus_manager_ensure_uid (ByxDBusManager          *self,
                                     GDBusMethodInvocation *context,
                                     gulong uid,
                                     GQuark error_domain,
                                     int error_code);

const char *byx_dbus_manager_connection_get_private_name (ByxDBusManager *self,
                                                         GDBusConnection *connection);

gboolean byx_dbus_manager_get_unix_user (ByxDBusManager *self,
                                        const char *sender,
                                        gulong *out_uid);

gboolean byx_dbus_manager_get_caller_info_from_message (ByxDBusManager *self,
                                                       GDBusConnection *connection,
                                                       GDBusMessage *message,
                                                       char **out_sender,
                                                       gulong *out_uid,
                                                       gulong *out_pid);

void byx_dbus_manager_private_server_register (ByxDBusManager *self,
                                              const char *path,
                                              const char *tag);

GDBusProxy *byx_dbus_manager_new_proxy (ByxDBusManager *self,
                                       GDBusConnection *connection,
                                       GType proxy_type,
                                       const char *name,
                                       const char *path,
                                       const char *iface);

#endif /* __BYX_DBUS_MANAGER_H__ */
