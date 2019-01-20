/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

#ifndef __BYX_CONNECTION_PLUGIN_INTERFACE_H__
#define __BYX_CONNECTION_PLUGIN_INTERFACE_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define BYX_TYPE_CONNECTION_PLUGIN (byx_connection_plugin_get_type ())

G_DECLARE_INTERFACE (ByxConnectionPlugin, byx_connection_plugin, BYX, CONNECTION_PLUGIN, GObject)

struct _ByxConnectionPluginInterface
{
	GTypeInterface g_iface;

	/* Virtual public methods */
	void (*activate) (ByxConnectionPlugin *plugin, ByxConnection *connection);
	void (*deactivate) (ByxConnectionPlugin *plugin, ByxConnection *connection);
};

void byx_connection_plugin_activate	(ByxConnectionPlugin *plugin, ByxConnection *connection);
void byx_connection_plugin_deactivate (ByxConnectionPlugin *plugin, ByxConnection *connection);

G_END_DECLS

#endif /* __BYX_CONNECTION_PLUGIN_INTERFACE_H__ */
