/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

#include "byx-common.h"

#include <assert.h>
#include <string.h>
#include <libpeas/peas.h>
#include "byx-plugin-manager.h"

struct _ByxPluginManagerClass {
	GObjectClass parent;
};

typedef struct {
	PeasEngine *engine;
} ByxPluginManagerPrivate;

struct _ByxPluginManager {
	GObject parent;
	ByxPluginManagerPrivate _priv;
};

G_DEFINE_TYPE (ByxPluginManager, byx_plugin_manager, G_TYPE_OBJECT)

/*****************************************************************************/

static ByxPluginManager *_singleton_instance = NULL;

ByxPluginManager *byx_plugin_manager_get (void)
{
    if (_singleton_instance == NULL) {
        _singleton_instance = g_object_new (BYX_TYPE_PLUGIN_MANAGER, NULL);
        assert (_singleton_instance != NULL);
    }

    return _singleton_instance;
}

/*****************************************************************************/

static void byx_plugin_manager_dispose (GObject *object);

static void byx_plugin_manager_class_init (ByxPluginManagerClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = byx_plugin_manager_dispose;
}

static void byx_plugin_manager_init (ByxPluginManager *self)
{
	ByxPluginManagerPrivate *priv = byx_plugin_manager_get_instance_private (self);

	priv->engine = peas_engine_new();
	peas_engine_enable_loader(priv->engine, "python3");
	peas_engine_add_search_path(priv->engine, "", "");			/* fixme */
}

static void byx_plugin_manager_dispose (GObject *object)
{
	ByxPluginManager *self = (ByxPluginManager *) object;
	ByxPluginManagerPrivate *priv = byx_plugin_manager_get_instance_private (self);

	g_object_unref (priv->engine);

	G_OBJECT_CLASS (byx_plugin_manager_parent_class)->dispose (object);
}

/*****************************************************************************/

