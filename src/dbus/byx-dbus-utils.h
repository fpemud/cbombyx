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
 * Copyright 2018 Red Hat, Inc.
 */

#ifndef __BYX_DBUS_UTILS_H__
#define __BYX_DBUS_UTILS_H__

/*****************************************************************************/

GDBusPropertyInfo *nm_dbus_utils_interface_info_lookup_property (const GDBusInterfaceInfo *interface_info,
                                                                 const char *property_name,
                                                                 guint *property_idx);

GDBusMethodInfo *nm_dbus_utils_interface_info_lookup_method (const GDBusInterfaceInfo *interface_info,
                                                             const char *method_name);

GVariant *nm_dbus_utils_get_property (GObject *obj,
                                      const char *signature,
                                      const char *property_name);

/*****************************************************************************/

void nm_dbus_utils_g_value_set_object_path (GValue *value, gpointer object);

void nm_dbus_utils_g_value_set_object_path_still_exported (GValue *value, gpointer object);

void nm_dbus_utils_g_value_set_object_path_from_hash (GValue *value,
                                                      GHashTable *hash,
                                                      gboolean expect_all_exported);

/*****************************************************************************/

typedef struct {
	union {
		gpointer const obj;
		gpointer _obj;
	};
	GObject *_notify_target;
	const GParamSpec *_notify_pspec;
	gulong _notify_signal_id;
	union {
		const bool visible;
		bool _visible;
	};
} NMDBusTrackObjPath;

void nm_dbus_track_obj_path_init (NMDBusTrackObjPath *track,
                                  GObject *target,
                                  const GParamSpec *pspec);

void nm_dbus_track_obj_path_deinit (NMDBusTrackObjPath *track);

void nm_dbus_track_obj_path_notify (const NMDBusTrackObjPath *track);

const char *nm_dbus_track_obj_path_get (const NMDBusTrackObjPath *track);

void nm_dbus_track_obj_path_set (NMDBusTrackObjPath *track,
                                 gpointer obj,
                                 gboolean visible);

#endif /* __BYX_DBUS_UTILS_H__ */
