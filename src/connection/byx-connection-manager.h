/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

#ifndef __BYX_CONNECTION_MANAGER_H__
#define __BYX_CONNECTION_MANAGER_H__

#include "byx-connection-plugin.h"
#include "byx-connection.h"

#define BYX_TYPE_CONNECTION_MANAGER            (byx_connection_manager_get_type ())
#define BYX_CONNECTION_MANAGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BYX_TYPE_CONNECTION_MANAGER, ByxConnectionManager))
#define BYX_CONNECTION_MANAGER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BYX_TYPE_CONNECTION_MANAGER, ByxConnectionManagerClass))
#define BYX_IS_CONNECTION_MANAGER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BYX_TYPE_CONNECTION_MANAGER))
#define BYX_IS_CONNECTION_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BYX_TYPE_CONNECTION_MANAGER))
#define BYX_CONNECTION_MANAGER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BYX_TYPE_CONNECTION_MANAGER, ByxConnectionManagerClass))

typedef struct _ByxConnectionManager ByxConnectionManager;
typedef struct _ByxConnectionManagerClass ByxConnectionManagerClass;

GType byx_connection_manager_get_type(void);

ByxConnectionManager *byx_connection_manager_get(void);

/*****************************************************************************/

gboolean byx_connection_manager_start (ByxConnectionManager *self, GError **error);

GSList *byx_connection_manager_get_all_plugins(ByxConnectionManager *self);

GSList *byx_connection_manager_get_activatable_connections(ByxConnectionManager *self,
                                                           gboolean sort);

gboolean byx_connection_manager_activate_connection(ByxConnectionManager *self,
                                                    ByxConnection *connection,
                                                    GError **error);

ByxConnection *byx_connection_manager_get_active_connection(ByxConnectionManager *self);

gboolean byx_connection_manager_deactivate_all_connections(ByxConnectionManager *self,
                                                           GError **error);

#endif /* __BYX_CONNECTION_MANAGER_H__ */
