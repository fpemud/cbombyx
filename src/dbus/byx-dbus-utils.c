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

#include "byx-common.h"

#include "nm-dbus-utils.h"

#include "nm-dbus-object.h"

/*****************************************************************************/

const GDBusSignalInfo nm_signal_info_property_changed_legacy = NM_DEFINE_GDBUS_SIGNAL_INFO_INIT (
	"PropertiesChanged",
	.args = NM_DEFINE_GDBUS_ARG_INFOS (
		NM_DEFINE_GDBUS_ARG_INFO ("properties", "a{sv}"),
	),
);

GDBusPropertyInfo *
nm_dbus_utils_interface_info_lookup_property (const GDBusInterfaceInfo *interface_info,
                                              const char *property_name,
                                              guint *property_idx)
{
	guint i;

	nm_assert (interface_info);
	nm_assert (property_name);

	/* there is also g_dbus_interface_info_lookup_property(), however that makes use
	 * of a global cache. */
	if (interface_info->properties) {
		for (i = 0; interface_info->properties[i]; i++) {
			GDBusPropertyInfo *info = interface_info->properties[i];

			if (byx_streq (info->name, property_name)) {
				NM_SET_OUT (property_idx, i);
				return info;
			}
		}
	}

	return NULL;
}

GDBusMethodInfo *
nm_dbus_utils_interface_info_lookup_method (const GDBusInterfaceInfo *interface_info,
                                            const char *method_name)
{
	guint i;

	nm_assert (interface_info);
	nm_assert (method_name);

	/* there is also g_dbus_interface_info_lookup_property(), however that makes use
	 * of a global cache. */
	if (interface_info->methods) {
		for (i = 0; interface_info->methods[i]; i++) {
			GDBusMethodInfo *info = interface_info->methods[i];

			if (byx_streq (info->name, method_name))
				return info;
		}
	}

	return NULL;
}

GVariant *
nm_dbus_utils_get_property (GObject *obj,
                            const char *signature,
                            const char *property_name)
{
	GParamSpec *pspec;
	nm_auto_unset_gvalue GValue value = G_VALUE_INIT;

	nm_assert (G_IS_OBJECT (obj));
	nm_assert (g_variant_type_string_is_valid (signature));
	nm_assert (property_name && property_name[0]);

	pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (obj), property_name);
	if (!pspec)
		g_return_val_if_reached (NULL);

	g_value_init (&value, pspec->value_type);
	g_object_get_property (obj, property_name, &value);
	/* returns never-floating variant */
	return g_dbus_gvalue_to_gvariant (&value, G_VARIANT_TYPE (signature));
}

/*****************************************************************************/

void
nm_dbus_utils_g_value_set_object_path (GValue *value, gpointer object)
{
	const char *path;

	g_return_if_fail (!object || NM_IS_DBUS_OBJECT (object));

	if (   object
	    && (path = nm_dbus_object_get_path (object)))
		g_value_set_string (value, path);
	else
		g_value_set_string (value, NULL);
}

void
nm_dbus_utils_g_value_set_object_path_still_exported (GValue *value, gpointer object)
{
	const char *path;

	g_return_if_fail (!object || NM_IS_DBUS_OBJECT (object));

	if (   object
	    && (path = nm_dbus_object_get_path_still_exported (object)))
		g_value_set_string (value, path);
	else
		g_value_set_string (value, "/");
}

void
nm_dbus_utils_g_value_set_object_path_from_hash (GValue *value,
                                                 GHashTable *hash /* has keys of ByxDBusObject type. */,
                                                 gboolean expect_all_exported)
{
	ByxDBusObject *obj;
	char **strv;
	guint i, n;
	GHashTableIter iter;

	nm_assert (value);
	nm_assert (hash);

	n = g_hash_table_size (hash);
	strv = g_new (char *, n + 1);
	i = 0;
	g_hash_table_iter_init (&iter, hash);
	while (g_hash_table_iter_next (&iter, (gpointer *) &obj, NULL)) {
		const char *path;

		path = nm_dbus_object_get_path (obj);
		if (!path) {
			nm_assert (!expect_all_exported);
			continue;
		}
		strv[i++] = g_strdup (path);
	}
	nm_assert (i <= n);
	strv[i] = NULL;

	/* sort the names, to give a well-defined, stable order. */
	byx_utils_strv_sort (strv, i);

	g_value_take_boxed (value, strv);
}

/*****************************************************************************/

void
nm_dbus_track_obj_path_init (NMDBusTrackObjPath *track,
                             GObject *target,
                             const GParamSpec *pspec)
{
	nm_assert (track);
	nm_assert (G_IS_OBJECT (target));
	nm_assert (G_IS_PARAM_SPEC (pspec));

	track->_obj = NULL;
	track->_notify_target = target;
	track->_notify_pspec = pspec;
	track->_notify_signal_id = 0;
	track->_visible = FALSE;
}

void
nm_dbus_track_obj_path_deinit (NMDBusTrackObjPath *track)
{
	/* we allow deinit() to be called multiple times (e.g. from
	 * dispose(), which must be re-entrant). */
	nm_assert (track);
	nm_assert (!track->_notify_target || G_IS_OBJECT (track->_notify_target));

	nm_clear_g_signal_handler (track->obj, &track->_notify_signal_id);
	track->_notify_target = NULL;
	track->_notify_pspec = NULL;
	track->_visible = FALSE;
	nm_clear_g_object (&track->_obj);
}

void
nm_dbus_track_obj_path_notify (const NMDBusTrackObjPath *track)
{
	nm_assert (track);
	nm_assert (G_IS_OBJECT (track->_notify_target));
	nm_assert (G_IS_PARAM_SPEC (track->_notify_pspec));

	g_object_notify_by_pspec (track->_notify_target,
	                          (GParamSpec *) track->_notify_pspec);
}

const char *
nm_dbus_track_obj_path_get (const NMDBusTrackObjPath *track)
{
	nm_assert (track);
	nm_assert (G_IS_OBJECT (track->_notify_target));

	return track->obj && track->visible
	       ? nm_dbus_object_get_path_still_exported (track->obj)
	       : NULL;
}

static void
_track_obj_exported_changed (ByxDBusObject *obj,
                             NMDBusTrackObjPath *track)
{
	nm_dbus_track_obj_path_notify (track);
}

void
nm_dbus_track_obj_path_set (NMDBusTrackObjPath *track,
                            gpointer obj,
                            gboolean visible)
{
	gs_unref_object ByxDBusObject *old_obj = NULL;
	const char *old_path;

	nm_assert (track);
	nm_assert (G_IS_OBJECT (track->_notify_target));

	g_return_if_fail (!obj || NM_IS_DBUS_OBJECT (obj));

	if (   track->obj == obj
	    && track->visible == !!visible)
		return;

	old_path = nm_dbus_track_obj_path_get (track);

	track->_visible = visible;

	if (track->obj != obj) {
		nm_clear_g_signal_handler (track->obj, &track->_notify_signal_id);

		old_obj = track->obj;
		track->_obj = nm_g_object_ref (obj);

		if (obj) {
			track->_notify_signal_id = g_signal_connect (obj,
			                                             NM_DBUS_OBJECT_EXPORTED_CHANGED,
			                                             G_CALLBACK (_track_obj_exported_changed),
			                                             track);
		}
	}

	if (!byx_streq0 (old_path, nm_dbus_track_obj_path_get (track)))
		nm_dbus_track_obj_path_notify (track);
}
