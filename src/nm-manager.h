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
 * Copyright (C) 2007 - 2008 Novell, Inc.
 * Copyright (C) 2007 - 2010 Red Hat, Inc.
 */

#ifndef __BYX_MANAGER_H__
#define __BYX_MANAGER_H__

#include "settings/nm-settings-connection.h"
#include "c-list/src/c-list.h"
#include "nm-dbus-manager.h"

#define BYX_TYPE_MANAGER            (byx_manager_get_type ())
#define BYX_MANAGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BYX_TYPE_MANAGER, ByxManager))
#define BYX_MANAGER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BYX_TYPE_MANAGER, ByxManagerClass))
#define BYX_IS_MANAGER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BYX_TYPE_MANAGER))
#define BYX_IS_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BYX_TYPE_MANAGER))
#define BYX_MANAGER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BYX_TYPE_MANAGER, ByxManagerClass))

#define BYX_MANAGER_VERSION "version"
#define BYX_MANAGER_CAPABILITIES "capabilities"
#define BYX_MANAGER_STATE "state"
#define BYX_MANAGER_STARTUP "startup"
#define BYX_MANAGER_NETWORKING_ENABLED "networking-enabled"
#define BYX_MANAGER_WIRELESS_ENABLED "wireless-enabled"
#define BYX_MANAGER_WIRELESS_HARDWARE_ENABLED "wireless-hardware-enabled"
#define BYX_MANAGER_WWAN_ENABLED "wwan-enabled"
#define BYX_MANAGER_WWAN_HARDWARE_ENABLED "wwan-hardware-enabled"
#define BYX_MANAGER_WIMAX_ENABLED "wimax-enabled"
#define BYX_MANAGER_WIMAX_HARDWARE_ENABLED "wimax-hardware-enabled"
#define BYX_MANAGER_ACTIVE_CONNECTIONS "active-connections"
#define BYX_MANAGER_CONNECTIVITY "connectivity"
#define BYX_MANAGER_CONNECTIVITY_CHECK_AVAILABLE "connectivity-check-available"
#define BYX_MANAGER_CONNECTIVITY_CHECK_ENABLED "connectivity-check-enabled"
#define BYX_MANAGER_PRIMARY_CONNECTION "primary-connection"
#define BYX_MANAGER_PRIMARY_CONNECTION_TYPE "primary-connection-type"
#define BYX_MANAGER_ACTIVATING_CONNECTION "activating-connection"
#define BYX_MANAGER_DEVICES "devices"
#define BYX_MANAGER_METERED "metered"
#define BYX_MANAGER_GLOBAL_DNS_CONFIGURATION "global-dns-configuration"
#define BYX_MANAGER_ALL_DEVICES "all-devices"

/* Not exported */
#define BYX_MANAGER_SLEEPING "sleeping"

/* Signals */
#define BYX_MANAGER_DEVICE_ADDED              "device-added"
#define BYX_MANAGER_DEVICE_REMOVED            "device-removed"
#define BYX_MANAGER_USER_PERMISSIONS_CHANGED  "user-permissions-changed"

#define BYX_MANAGER_ACTIVE_CONNECTION_ADDED   "active-connection-added"
#define BYX_MANAGER_ACTIVE_CONNECTION_REMOVED "active-connection-removed"
#define BYX_MANAGER_INTERNAL_DEVICE_ADDED     "internal-device-added"
#define BYX_MANAGER_INTERNAL_DEVICE_REMOVED   "internal-device-removed"

GType byx_manager_get_type (void);

/* byx_manager_setup() should only be used by main.c */
ByxManager *  byx_manager_setup                         (void);

ByxManager *  byx_manager_get                           (void);

gboolean      byx_manager_start                         (ByxManager *manager, GError **error);

void          byx_manager_stop                          (ByxManager *manager);

NMState       byx_manager_get_state                     (ByxManager *manager);

const CList * byx_manager_get_active_connections        (ByxManager *manager);

#define byx_manager_for_each_active_connection(manager, iter, tmp_list) \
	for (tmp_list = byx_manager_get_active_connections (manager), \
	     iter = c_list_entry (tmp_list->next, NMActiveConnection, active_connections_lst); \
	     ({ \
	         const gboolean _has_next = (&iter->active_connections_lst != tmp_list); \
	         \
	         if (!_has_next) \
	             iter = NULL; \
	         _has_next; \
	    }); \
	    iter = c_list_entry (iter->active_connections_lst.next, NMActiveConnection, active_connections_lst))

NMSettingsConnection **byx_manager_get_activatable_connections (ByxManager *manager,
                                                               guint *out_len,
                                                               gboolean sort);

/* Device handling */

const CList *       byx_manager_get_devices             (ByxManager *manager);

#define byx_manager_for_each_device(manager, iter, tmp_list) \
	for (tmp_list = byx_manager_get_devices (manager), \
	     iter = c_list_entry (tmp_list->next, NMDevice, devices_lst); \
	     ({ \
	         const gboolean _has_next = (&iter->devices_lst != tmp_list); \
	         \
	         if (!_has_next) \
	             iter = NULL; \
	         _has_next; \
	    }); \
	    iter = c_list_entry (iter->devices_lst.next, NMDevice, devices_lst))

NMDevice *          byx_manager_get_device_by_ifindex   (ByxManager *manager,
                                                        int ifindex);
NMDevice *          byx_manager_get_device_by_path      (ByxManager *manager,
                                                        const char *path);

guint32             byx_manager_device_route_metric_reserve (ByxManager *self,
                                                            int ifindex,
                                                            NMDeviceType device_type);

void                byx_manager_device_route_metric_clear (ByxManager *self,
                                                          int ifindex);

char *              byx_manager_get_connection_iface (ByxManager *self,
                                                     NMConnection *connection,
                                                     NMDevice **out_parent,
                                                     GError **error);

const char *        byx_manager_iface_for_uuid          (ByxManager *self,
                                                        const char *uuid);

NMActiveConnection *byx_manager_activate_connection     (ByxManager *manager,
                                                        NMSettingsConnection *connection,
                                                        NMConnection *applied_connection,
                                                        const char *specific_object,
                                                        NMDevice *device,
                                                        NMAuthSubject *subject,
                                                        NMActivationType activation_type,
                                                        NMActivationReason activation_reason,
                                                        GError **error);

gboolean            byx_manager_deactivate_connection   (ByxManager *manager,
                                                        NMActiveConnection *active,
                                                        NMDeviceStateReason reason,
                                                        GError **error);

void                byx_manager_set_capability   (ByxManager *self, NMCapability cap);

NMDevice *          byx_manager_get_device    (ByxManager *self,
                                              const char *ifname,
                                              NMDeviceType device_type);
gboolean            byx_manager_remove_device (ByxManager *self,
                                              const char *ifname,
                                              NMDeviceType device_type);

void byx_manager_dbus_set_property_handle (ByxDBusObject *obj,
                                          const ByxDBusInterfaceInfoExtended *interface_info,
                                          const ByxDBusPropertyInfoExtended *property_info,
                                          GDBusConnection *connection,
                                          const char *sender,
                                          GDBusMethodInvocation *invocation,
                                          GVariant *value,
                                          gpointer user_data);

#endif /* __BYX_MANAGER_H__ */
