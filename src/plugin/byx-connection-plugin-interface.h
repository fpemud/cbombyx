/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

#ifndef __BYX_CONNECTION_PLUGIN_INTERFACE_H__
#define __BYX_CONNECTION_PLUGIN_INTERFACE_H__

#include <glib-object.h>

G_BEGIN_DECLS

typdef struct {

} ByxConnectionPluginWirelessParam;

typdef struct {

} ByxConnectionPluginDhcpParam;

#define BYX_TYPE_CONNECTION_PLUGIN (byx_connection_plugin_get_type ())

G_DECLARE_INTERFACE (ByxConnectionPlugin, byx_connection_plugin, BYX, CONNECTION_PLUGIN, GObject)

struct _ByxConnectionPluginInterface
{
	GTypeInterface g_iface;

	/* General */
	ByxPluginFeature (*belong_to_network_type) (ByxConnection *plugin, ByxNetworkType network_type);

	/* Physical layer */
	ByxPluginFeature (*ignore_carrier)   (ByxConnectionPlugin *plugin);

	/* Data-link layer (general) */
	ByxPluginFeature (*need_fake_hwaddr) (ByxConnectionPlugin *plugin);

	/* Data-link layer (wireless) */
	ByxPluginFeature                        (*depend_on_wireless_scan)            (ByxConnectionPlugin *plugin);
	gboolean                                (*check_wireless_scan)                (ByxConnectionPlugin *plugin);
	gboolean                                (*check_wireless_scan_for_connection) (ByxConnectionPlugin *plugin, ByxConnection *connection);
	const ByxConnectionPluginWirelessParam* (*get_wireless_param)                 (ByxConnectionPlugin *plugin, ByxConnection *connection);

	/* Network layer */
	ByxPluginFeature                    (*depend_on_dhcp_discovery)            (ByxConnectionPlugin *plugin);
	gboolean                            (*check_dhcp_discovery)                (ByxConnectionPlugin *plugin);
	gboolean                            (*check_dhcp_discovery_for_connection) (ByxConnectionPlugin *plugin, ByxConnection *connection);
	const ByxConnectionPluginDhcpParam* (*get_dhcp_param)                      (ByxConnectionPlugin *plugin, ByxConnection *connection);
	gboolean                            (*check_dhcp_lease_for_connection)     (ByxConnectionPlugin *plugin, ByxConnection *connection);

	/* */
	gboolean (*activate) (ByxConnectionPlugin *plugin, ByxConnection *connection, char *reason);		/* FIXME */
	void (*deactivate) (ByxConnectionPlugin *plugin, ByxConnection *connection);
};

void byx_connection_plugin_activate	(ByxConnectionPlugin *plugin, ByxConnection *connection);
void byx_connection_plugin_deactivate (ByxConnectionPlugin *plugin, ByxConnection *connection);

G_END_DECLS

#endif /* __BYX_CONNECTION_PLUGIN_INTERFACE_H__ */
