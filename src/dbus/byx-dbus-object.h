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

#ifndef __BYX_DBUS_OBJECT_H__
#define __BYX_DBUS_OBJECT_H__

/*****************************************************************************/

#include "c-list/src/c-list.h"
#include "nm-dbus-utils.h"

/*****************************************************************************/

typedef struct {
	const char *path;

	/* if path is of type NM_DBUS_EXPORT_PATH_NUMBERED(), we need a
	 * per-class counter when generating a new numbered path.
	 *
	 * Each ByxDBusObjectClass instance has a shallow clone of the ByxDBusObjectClass parent
	 * instance in every derived type. Hence we cannot embed the counter there directly,
	 * because it must be shared, e.g. between NMDeviceBond and NMDeviceEthernet.
	 * Make int_counter a pointer to the actual counter that is used by ever sibling
	 * class. */
	long long unsigned *int_counter;
} NMDBusExportPath;

#define NM_DBUS_EXPORT_PATH_STATIC(basepath) \
	({ \
		((NMDBusExportPath) { \
			.path = ""basepath"", \
		}); \
	})

#define NM_DBUS_EXPORT_PATH_NUMBERED(basepath) \
	({ \
		static long long unsigned _int_counter = 0; \
		((NMDBusExportPath) { \
			.path = ""basepath"/%llu", \
			.int_counter = &_int_counter, \
		}); \
	})

/*****************************************************************************/

/* "org.freedesktop.NetworkManager.Device.Statistics" is a special interface,
 * because although it has a legacy PropertiesChanged signal, it only notifies
 * about properties that actually exist on that interface. That is, because it
 * was added with 1.4.0 release, and thus didn't have the broken behavior like
 * other legacy interfaces. Those notify about *all* properties, even if they
 * are not part of that D-Bus interface. See also "include_in_legacy_property_changed"
 * and "legacy_property_changed". */
extern const ByxDBusInterfaceInfoExtended nm_interface_info_device_statistics;

/*****************************************************************************/

#define NM_TYPE_DBUS_OBJECT            (nm_dbus_object_get_type ())
#define NM_DBUS_OBJECT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), NM_TYPE_DBUS_OBJECT, ByxDBusObject))
#define NM_DBUS_OBJECT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  NM_TYPE_DBUS_OBJECT, ByxDBusObjectClass))
#define NM_IS_DBUS_OBJECT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NM_TYPE_DBUS_OBJECT))
#define NM_IS_DBUS_OBJECT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  NM_TYPE_DBUS_OBJECT))
#define NM_DBUS_OBJECT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  NM_TYPE_DBUS_OBJECT, ByxDBusObjectClass))

#define NM_DBUS_OBJECT_EXPORTED_CHANGED "exported-changed"

/* ByxDBusObject and ByxDBusManager cooperate strongly. Hence, there is an
 * internal data structure attached to the ByxDBusObject accessible to both of them. */
struct _ByxDBusObjectInternal {
	char *path;
	ByxDBusManager *bus_manager;
	CList objects_lst;
	CList registration_lst_head;

	/* we perform asynchronous operation on exported objects. For example, we receive
	 * a Set property call, and asynchronously validate the operation. We must make
	 * sure that when the authentication is complete, that we are still looking at
	 * the same (exported) object. In the meantime, the object could have been
	 * unexported, or even re-exported afterwards. If that happens, we want
	 * to fail the request. For that, we keep track of a version id.  */
	guint64 export_version_id;
	bool is_unexporting:1;
};

struct _ByxDBusObject {
	GObject parent;
	struct _ByxDBusObjectInternal internal;
};

#define NM_DEFINE_DBUS_INTERFACE_INFO(...) \
	((NMDBusInterfaceInfo *) (&((const NMDBusInterfaceInfo) { \
		__VA_ARGS__ \
	})))

typedef struct {
	GObjectClass parent;

	NMDBusExportPath export_path;

	const ByxDBusInterfaceInfoExtended *const*interface_infos;

	bool export_on_construction;
} ByxDBusObjectClass;

GType nm_dbus_object_get_type (void);

static inline ByxDBusManager *
nm_dbus_object_get_manager (ByxDBusObject *obj)
{
	nm_assert (NM_IS_DBUS_OBJECT (obj));

	return obj->internal.bus_manager;
}

static inline guint64
nm_dbus_object_get_export_version_id (ByxDBusObject *obj)
{
	nm_assert (NM_IS_DBUS_OBJECT (obj));

	return obj->internal.export_version_id;
}

/**
 * nm_dbus_object_get_path:
 * @self: an #ByxDBusObject
 *
 * Gets @self's D-Bus path.
 *
 * Returns: @self's D-Bus path, or %NULL if @self is not exported.
 */
static inline const char *
nm_dbus_object_get_path (ByxDBusObject *self)
{
	g_return_val_if_fail (NM_IS_DBUS_OBJECT (self), NULL);

	return self->internal.path;
}

/**
 * nm_dbus_object_is_exported:
 * @self: an #ByxDBusObject
 *
 * Checks if @self is exported
 *
 * Returns: %TRUE if @self is exported
 */
static inline gboolean
nm_dbus_object_is_exported (ByxDBusObject *self)
{
	return !!nm_dbus_object_get_path (self);
}

static inline const char *
nm_dbus_object_get_path_still_exported (ByxDBusObject *self)
{
	g_return_val_if_fail (NM_IS_DBUS_OBJECT (self), NULL);

	/* like nm_dbus_object_get_path(), however, while unexporting
	 * (exported-changed signal), returns %NULL instead of the path. */
	return self->internal.is_unexporting
	       ? NULL
	       : self->internal.path;
}

const char *nm_dbus_object_export      (ByxDBusObject *self);
void        nm_dbus_object_unexport    (ByxDBusObject *self);

void        _nm_dbus_object_clear_and_unexport (ByxDBusObject **location);
#define nm_dbus_object_clear_and_unexport(location) _nm_dbus_object_clear_and_unexport ((ByxDBusObject **) (location))

void        nm_dbus_object_emit_signal_variant (ByxDBusObject *self,
                                                const ByxDBusInterfaceInfoExtended *interface_info,
                                                const GDBusSignalInfo *signal_info,
                                                GVariant *args);

void        nm_dbus_object_emit_signal (ByxDBusObject *self,
                                        const ByxDBusInterfaceInfoExtended *interface_info,
                                        const GDBusSignalInfo *signal_info,
                                        const char *format,
                                        ...);

#endif /* __BYX_DBUS_OBJECT_H__ */
