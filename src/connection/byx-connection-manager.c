/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

#include "byx-common.h"

#include <assert.h>
#include <string.h>
#include "byx-connection.h"
#include "byx-connection-manager.h"

struct _ByxConnectionManagerClass {
	GObjectClass parent;
};

typedef struct {
	GSList *connection_list;
} ByxConnectionManagerPrivate;

struct _ByxConnectionManager {
	GObject parent;
	ByxConnectionManagerPrivate _priv;
};

G_DEFINE_TYPE_WITH_PRIVATE (ByxConnectionManager, byx_connection_manager, G_TYPE_OBJECT)

/*****************************************************************************/

static ByxConnectionManager *_singleton_instance = NULL;

ByxConnectionManager *byx_connection_manager_get (void)
{
    if (_singleton_instance == NULL) {
        _singleton_instance = g_object_new (BYX_TYPE_CONNECTION_MANAGER, NULL);
        assert (_singleton_instance != NULL);
    }

    return _singleton_instance;
}

/*****************************************************************************/

static void byx_connection_manager_dispose (GObject *object);

static void byx_connection_manager_class_init (ByxConnectionManagerClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = byx_connection_manager_dispose;
}

static void byx_connection_manager_init (ByxConnectionManager *self)
{
	ByxConnectionManagerPrivate *priv = byx_connection_manager_get_instance_private (self);

	priv->connection_list = NULL;									/* fixme */
}

static void byx_connection_manager_dispose (GObject *object)
{
	ByxConnectionManager *self = (ByxConnectionManager *) object;
	ByxConnectionManagerPrivate *priv = byx_connection_manager_get_instance_private (self);

	g_object_unref (priv->connection_list);

	G_OBJECT_CLASS (byx_connection_manager_parent_class)->dispose (object);
}

/*****************************************************************************/

GSList *byx_connection_manager_get_activatable_connections (ByxConnectionManager *manager,
                                                           gboolean sort)
{
	return NULL;
}

gboolean byx_connection_manager_activate_connection (ByxConnectionManager *manager,
                                                     ByxConnection *connection,
                                                     GError **error)
{
	return FALSE;
}

gboolean byx_connection_manager_deactivate_all_connections (ByxConnectionManager *manager,
                                                            GError **error)
{
	return FALSE;
}
