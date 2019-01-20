/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

#ifndef __BYX_SERVICE_PLUGIN_INTERFACE_H__
#define __BYX_SERVICE_PLUGIN_INTERFACE_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define BYX_TYPE_SERVICE_PLUGIN (byx_service_plugin_get_type ())

G_DECLARE_INTERFACE (ByxServicePlugin, byx_service_plugin, BYX, SERVICE_PLUGIN, GObject)

struct _ByxServicePluginInterface
{
	GTypeInterface g_iface;

	/* Virtual public methods */
	void (*start) (ByxServicePlugin *plugin, ByxService *service);
	void (*stop) (ByxServicePlugin *plugin, ByxService *service);
};

void byx_service_plugin_start (ByxServicePlugin *plugin, ByxService *service);
void byx_service_plugin_stop (ByxServicePlugin *plugin, ByxService *service);

G_END_DECLS

#endif /* __BYX_SERVICE_PLUGIN_INTERFACE_H__ */
