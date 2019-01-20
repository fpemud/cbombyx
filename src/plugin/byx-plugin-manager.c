/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

#include "byx-default.h"

#include <string.h>
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

#define BYX_CONNECTION_MANAGER_GET_PRIVATE(self) _BYX_GET_PRIVATE (self, ByxPluginManager, BYX_IS_CONNECTION_MANAGER)

/*****************************************************************************/

BYX_DEFINE_SINGLETON_GETTER (ByxPluginManager, byx_plugin_manager_get, BYX_TYPE_CONNECTION_MANAGER);
