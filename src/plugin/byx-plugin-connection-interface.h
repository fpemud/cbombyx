/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

#ifndef __BYX_PLUGIN_CONNECTION_INTERFACE_H__
#define __BYX_PLUGIN_CONNECTION_INTERFACE_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define BYX_TYPE_PLUGIN_CONNECTION (byx_plugin_connection_get_type ())

G_DECLARE_INTERFACE (ByxPluginConnection, byx_plugin_connection, BYX, PLUGIN_CONNECTION, GObject)

struct _ByxPluginConnectionInterface
{
	GTypeInterface g_iface;

	/* Virtual public methods */
	void (*activate) (ByxPluginConnection *plugin, ByxConnection *connection);
	void (*deactivate) (ByxPluginConnection *plugin, ByxConnection *connection);
};

void byx_plugin_connection_activate	(ByxPluginConnection *plugin, ByxConnection *connection);
void byx_plugin_connection_deactivate (ByxPluginConnection *plugin, ByxConnection *connection);

G_END_DECLS

#endif /* __BYX_PLUGIN_CONNECTION_INTERFACE_H__ */
