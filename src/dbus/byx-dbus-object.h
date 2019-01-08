/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

#ifndef __BYX_DBUS_OBJECT_H__
#define __BYX_DBUS_OBJECT_H__

#include "byx-dbus-common.h"

#define BYX_TYPE_DBUS_OBJECT            (byx_dbus_object_get_type ())
#define BYX_DBUS_OBJECT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BYX_TYPE_DBUS_OBJECT, ByxDBusObject))
#define BYX_DBUS_OBJECT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  BYX_TYPE_DBUS_OBJECT, ByxDBusObjectClass))
#define BYX_IS_DBUS_OBJECT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BYX_TYPE_DBUS_OBJECT))
#define BYX_IS_DBUS_OBJECT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  BYX_TYPE_DBUS_OBJECT))
#define BYX_DBUS_OBJECT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  BYX_TYPE_DBUS_OBJECT, ByxDBusObjectClass))

typedef struct _ByxDBusObject ByxDBusObject;
typedef struct _ByxDBusObjectClass ByxDBusObjectClass;

GType byx_dbus_object_get_type (void);

ByxDBusManager *byx_dbus_object_get_manager (ByxDBusObject *obj);

/*
guint64 byx_dbus_object_get_export_version_id (ByxDBusObject *obj);

const char *byx_dbus_object_export      (ByxDBusObject *self);
void        byx_dbus_object_unexport    (ByxDBusObject *self);

void        byx_dbus_object_clear_and_unexport (ByxDBusObject **location);

void        byx_dbus_object_emit_signal_variant (ByxDBusObject *self,
                                                const ByxDBusInterfaceInfoExtended *interface_info,
                                                const GDBusSignalInfo *signal_info,
                                                GVariant *args);

void        byx_dbus_object_emit_signal (ByxDBusObject *self,
                                        const ByxDBusInterfaceInfoExtended *interface_info,
                                        const GDBusSignalInfo *signal_info,
                                        const char *format,
                                        ...);
*/

#endif /* __BYX_DBUS_OBJECT_H__ */
