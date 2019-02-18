/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

#ifndef __BYX_DBUS_MANAGER_H__
#define __BYX_DBUS_MANAGER_H__

#include "byx-dbus-common.h"
#include "byx-dbus-object.h"

#define BYX_TYPE_DBUS_MANAGER            (byx_dbus_manager_get_type ())
#define BYX_DBUS_MANAGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BYX_TYPE_DBUS_MANAGER, ByxDBusManager))
#define BYX_DBUS_MANAGER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), BYX_TYPE_DBUS_MANAGER, ByxDBusManagerClass))
#define BYX_IS_DBUS_MANAGER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BYX_TYPE_DBUS_MANAGER))
#define BYX_IS_DBUS_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BYX_TYPE_DBUS_MANAGER))
#define BYX_DBUS_MANAGER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BYX_TYPE_DBUS_MANAGER, ByxDBusManagerClass))

typedef struct _ByxDBusManager ByxDBusManager;
typedef struct _ByxDBusManagerClass ByxDBusManagerClass;

GType byx_dbus_manager_get_type (void);

ByxDBusManager *byx_dbus_manager_get (void);

/*****************************************************************************/

#define BYX_DBUS_MANAGER_PRIVATE_CONNECTION_NEW           "private-connection-new"
#define BYX_DBUS_MANAGER_PRIVATE_CONNECTION_DISCONNECTED  "private-connection-disconnected"

gboolean byx_dbus_manager_start (ByxDBusManager *self, GError **error);

void byx_dbus_manager_stop (ByxDBusManager *self);


#if 0
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
#endif

#endif /* __BYX_DBUS_MANAGER_H__ */
