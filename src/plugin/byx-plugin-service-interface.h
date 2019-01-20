/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

#ifndef __BYX_PLUGIN_SERVICE_INTERFACE_H__
#define __BYX_PLUGIN_SERVICE_INTERFACE_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define BYX_TYPE_PLUGIN_SERVICE (byx_plugin_service_get_type ())

G_DECLARE_INTERFACE (ByxPluginService, byx_plugin_service, BYX, PLUGIN_SERVICE, GObject)

struct _ByxPluginServiceInterface
{
	GTypeInterface g_iface;

	/* Virtual public methods */
	void (*start) (ByxPluginService *plugin, ByxService *service);
	void (*stop) (ByxPluginService *plugin, ByxService *service);
};

void byx_plugin_service_start (ByxPluginService *plugin, ByxService *service);
void byx_plugin_service_stop (ByxPluginService *plugin, ByxService *service);

G_END_DECLS

#endif /* __BYX_PLUGIN_SERVICE_INTERFACE_H__ */
