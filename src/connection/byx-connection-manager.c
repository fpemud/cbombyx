/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

#include "byx-default.h"

#include <string.h>
#include "byx-connection.h"
#include "byx-connection-manager.h"

struct _ByxConnectionManagerClass {
	GObjectClass parent;
};

typedef struct {
	PeasEngine *plugin_engine;
	GSList *connection_list;
} ByxConnectionManagerPrivate;

struct _ByxConnectionManager {
	GObject parent;
	ByxConnectionManagerPrivate _priv;
};

G_DEFINE_TYPE (ByxConnectionManager, byx_connection_manager, G_TYPE_OBJECT)

#define BYX_CONNECTION_MANAGER_GET_PRIVATE(self) _BYX_GET_PRIVATE (self, ByxConnectionManager, BYX_IS_CONNECTION_MANAGER)

/*****************************************************************************/

BYX_DEFINE_SINGLETON_GETTER (ByxConnectionManager, byx_connection_manager_get, BYX_TYPE_CONNECTION_MANAGER);

static void byx_connection_manager_class_init (ByxConnectionManagerClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = byx_connection_manager_dispose;
}

static void byx_connection_manager_init (ByxConnectionManager *self)
{
	ByxConnectionManagerPrivate *priv = BYX_CONNECTION_MANAGER_GET_PRIVATE (self);

	priv->plugin_engine = peas_engine_new();
	peas_engine_enable_loader(priv->plugin_engine, "python3");
	peas_engine_add_search_path(priv->plugin_engine, "");			/* fixme */

	priv->connection_list = NULL									/* fixme */




}

static void byx_connection_manager_dispose (GObject *object)
{
	ByxConnectionManager *self = (ByxConnectionManager *) object;
	ByxConnectionManagerPrivate *priv = BYX_CONNECTION_MANAGER_GET_PRIVATE (self);

	g_object_unref (priv->connection_list);
	g_object_unref (priv->plugin_engine);

	G_OBJECT_CLASS (byx_connection_manager_parent_class)->dispose (object);
}

/*****************************************************************************/

CList *byx_connection_manager_get_activatable_connections (ByxConnectionManager *manager,
                                                           gboolean sort)
{
	ByxConnectionManagerPrivate *priv = BYX_CONNECTION_MANAGER_GET_PRIVATE (self);

}

gboolean byx_connection_manager_activate_connection (ByxConnectionManager *manager,
                                                     ByxConnection *connection,
                                                     GError **error)
{
}

gboolean byx_connection_manager_deactivate_all_connections (ByxConnectionManager *manager,
                                                            GError **error)
{

}
