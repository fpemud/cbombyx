/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* NetworkManager system settings service
 *
 * Søren Sandmann <sandmann@daimi.au.dk>
 * Dan Williams <dcbw@redhat.com>
 * Tambet Ingo <tambet@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * (C) Copyright 2007 - 2011 Red Hat, Inc.
 * (C) Copyright 2008 Novell, Inc.
 */

#include "nm-default.h"

#include "nm-settings.h"

#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <gmodule.h>
#include <pwd.h>

#include "nm-common-macros.h"
#include "nm-dbus-interface.h"
#include "nm-connection.h"
#include "nm-setting-8021x.h"
#include "nm-setting-bluetooth.h"
#include "nm-setting-cdma.h"
#include "nm-setting-connection.h"
#include "nm-setting-gsm.h"
#include "nm-setting-ip4-config.h"
#include "nm-setting-ip6-config.h"
#include "nm-setting-olpc-mesh.h"
#include "nm-setting-ppp.h"
#include "nm-setting-pppoe.h"
#include "nm-setting-serial.h"
#include "nm-setting-vpn.h"
#include "nm-setting-wired.h"
#include "nm-setting-adsl.h"
#include "nm-setting-wireless.h"
#include "nm-setting-wireless-security.h"
#include "nm-setting-proxy.h"
#include "nm-setting-bond.h"
#include "nm-utils.h"
#include "nm-core-internal.h"

#include "nm-utils/nm-c-list.h"
#include "nm-dbus-object.h"
#include "devices/nm-device-ethernet.h"
#include "nm-settings-connection.h"
#include "nm-settings-plugin.h"
#include "nm-dbus-manager.h"
#include "nm-auth-utils.h"
#include "nm-auth-subject.h"
#include "nm-session-monitor.h"
#include "plugins/keyfile/nms-keyfile-plugin.h"
#include "nm-agent-manager.h"
#include "nm-config.h"
#include "nm-audit-manager.h"
#include "NetworkManagerUtils.h"
#include "nm-hostname-manager.h"

/*****************************************************************************/

#define EXPORT(sym) void * __export_##sym = &sym;

EXPORT(nm_settings_connection_get_type)
EXPORT(nm_settings_connection_update)

/*****************************************************************************/

static NM_CACHED_QUARK_FCN ("plugin-module-path", plugin_module_path_quark)
static NM_CACHED_QUARK_FCN ("default-wired-connection", _default_wired_connection_quark)
static NM_CACHED_QUARK_FCN ("default-wired-device", _default_wired_device_quark)

/*****************************************************************************/

NM_GOBJECT_PROPERTIES_DEFINE (NMSettings,
	PROP_UNMANAGED_SPECS,
	PROP_CAN_MODIFY,
	PROP_CONNECTIONS,
	PROP_STARTUP_COMPLETE,
);

enum {
	CONNECTION_ADDED,
	CONNECTION_UPDATED,
	CONNECTION_REMOVED,
	CONNECTION_FLAGS_CHANGED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

typedef struct {
	NMAgentManager *agent_mgr;

	NMConfig *config;

	GSList *auths;

	GSList *plugins;

	CList connections_lst_head;

	NMSettingsConnection **connections_cached_list;
	GSList *unmanaged_specs;
	GSList *unrecognized_specs;

	NMHostnameManager *hostname_manager;

	guint connections_len;

	bool started:1;
	bool startup_complete:1;
	bool connections_loaded:1;

} NMSettingsPrivate;

struct _NMSettings {
	ByxDBusObject parent;
	NMSettingsPrivate _priv;
};

struct _NMSettingsClass {
	ByxDBusObjectClass parent;
};

G_DEFINE_TYPE (NMSettings, nm_settings, NM_TYPE_DBUS_OBJECT);

#define NM_SETTINGS_GET_PRIVATE(self) _BYX_GET_PRIVATE (self, NMSettings, NM_IS_SETTINGS)

/*****************************************************************************/

#define _NMLOG_DOMAIN         LOGD_SETTINGS
#define _NMLOG(level, ...) __NMLOG_DEFAULT (level, _NMLOG_DOMAIN, "settings", __VA_ARGS__)

/*****************************************************************************/

static const ByxDBusInterfaceInfoExtended interface_info_settings;
static const GDBusSignalInfo signal_info_new_connection;
static const GDBusSignalInfo signal_info_connection_removed;

static void claim_connection (NMSettings *self,
                              NMSettingsConnection *connection);

static void unmanaged_specs_changed (NMSettingsPlugin *config, gpointer user_data);
static void unrecognized_specs_changed (NMSettingsPlugin *config, gpointer user_data);

static void connection_ready_changed (NMSettingsConnection *conn,
                                      GParamSpec *pspec,
                                      gpointer user_data);

static void default_wired_clear_tag (NMSettings *self,
                                     NMDevice *device,
                                     NMSettingsConnection *connection,
                                     gboolean add_to_no_auto_default);

/*****************************************************************************/

static void
check_startup_complete (NMSettings *self)
{
	NMSettingsPrivate *priv = NM_SETTINGS_GET_PRIVATE (self);
	NMSettingsConnection *conn;

	if (priv->startup_complete)
		return;

	c_list_for_each_entry (conn, &priv->connections_lst_head, _connections_lst) {
		if (!nm_settings_connection_get_ready (conn))
			return;
	}

	/* the connection_ready_changed signal handler is no longer needed. */
	c_list_for_each_entry (conn, &priv->connections_lst_head, _connections_lst)
		g_signal_handlers_disconnect_by_func (conn, G_CALLBACK (connection_ready_changed), self);

	priv->startup_complete = TRUE;
	_notify (self, PROP_STARTUP_COMPLETE);
}

static void
connection_ready_changed (NMSettingsConnection *conn,
                          GParamSpec *pspec,
                          gpointer user_data)
{
	NMSettings *self = NM_SETTINGS (user_data);

	if (nm_settings_connection_get_ready (conn))
		check_startup_complete (self);
}

static void
plugin_connection_added (NMSettingsPlugin *config,
                         NMSettingsConnection *connection,
                         gpointer user_data)
{
	claim_connection (NM_SETTINGS (user_data), connection);
}

static void
load_connections (NMSettings *self)
{
	NMSettingsPrivate *priv = NM_SETTINGS_GET_PRIVATE (self);
	GSList *iter;

	for (iter = priv->plugins; iter; iter = g_slist_next (iter)) {
		NMSettingsPlugin *plugin = NM_SETTINGS_PLUGIN (iter->data);
		GSList *plugin_connections;
		GSList *elt;

		plugin_connections = nm_settings_plugin_get_connections (plugin);

		// FIXME: ensure connections from plugins loaded with a lower priority
		// get rejected when they conflict with connections from a higher
		// priority plugin.

		for (elt = plugin_connections; elt; elt = g_slist_next (elt))
			claim_connection (self, NM_SETTINGS_CONNECTION (elt->data));

		g_slist_free (plugin_connections);

		g_signal_connect (plugin, NM_SETTINGS_PLUGIN_CONNECTION_ADDED,
		                  G_CALLBACK (plugin_connection_added), self);
		g_signal_connect (plugin, NM_SETTINGS_PLUGIN_UNMANAGED_SPECS_CHANGED,
		                  G_CALLBACK (unmanaged_specs_changed), self);
		g_signal_connect (plugin, NM_SETTINGS_PLUGIN_UNRECOGNIZED_SPECS_CHANGED,
		                  G_CALLBACK (unrecognized_specs_changed), self);
	}

	priv->connections_loaded = TRUE;
	_notify (self, PROP_CONNECTIONS);

	unmanaged_specs_changed (NULL, self);
	unrecognized_specs_changed (NULL, self);
}

static void
impl_settings_list_connections (ByxDBusObject *obj,
                                const ByxDBusInterfaceInfoExtended *interface_info,
                                const NMDBusMethodInfoExtended *method_info,
                                GDBusConnection *dbus_connection,
                                const char *sender,
                                GDBusMethodInvocation *invocation,
                                GVariant *parameters)
{
	NMSettings *self = NM_SETTINGS (obj);
	NMSettingsPrivate *priv = NM_SETTINGS_GET_PRIVATE (self);
	gs_free const char **strv = NULL;

	strv = nm_dbus_utils_get_paths_for_clist (&priv->connections_lst_head,
	                                          priv->connections_len,
	                                          G_STRUCT_OFFSET (NMSettingsConnection, _connections_lst),
	                                          TRUE);
	g_dbus_method_invocation_return_value (invocation,
	                                       g_variant_new ("(^ao)", strv));
}

NMSettingsConnection *
nm_settings_get_connection_by_uuid (NMSettings *self, const char *uuid)
{
	NMSettingsPrivate *priv;
	NMSettingsConnection *candidate;

	g_return_val_if_fail (NM_IS_SETTINGS (self), NULL);
	g_return_val_if_fail (uuid != NULL, NULL);

	priv = NM_SETTINGS_GET_PRIVATE (self);

	c_list_for_each_entry (candidate, &priv->connections_lst_head, _connections_lst) {
		if (nm_streq (uuid, nm_settings_connection_get_uuid (candidate)))
			return candidate;
	}

	return NULL;
}

static void
impl_settings_get_connection_by_uuid (ByxDBusObject *obj,
                                      const ByxDBusInterfaceInfoExtended *interface_info,
                                      const NMDBusMethodInfoExtended *method_info,
                                      GDBusConnection *dbus_connection,
                                      const char *sender,
                                      GDBusMethodInvocation *invocation,
                                      GVariant *parameters)
{
	NMSettings *self = NM_SETTINGS (obj);
	NMSettingsConnection *connection = NULL;
	gs_unref_object NMAuthSubject *subject = NULL;
	GError *error = NULL;
	const char *uuid;

	g_variant_get (parameters, "(&s)", &uuid);

	connection = nm_settings_get_connection_by_uuid (self, uuid);
	if (!connection) {
		error = g_error_new_literal (NM_SETTINGS_ERROR,
		                             NM_SETTINGS_ERROR_INVALID_CONNECTION,
		                             "No connection with the UUID was found.");
		goto error;
	}

	subject = nm_auth_subject_new_unix_process_from_context (invocation);
	if (!subject) {
		error = g_error_new_literal (NM_SETTINGS_ERROR,
		                             NM_SETTINGS_ERROR_PERMISSION_DENIED,
		                             "Unable to determine UID of request.");
		goto error;
	}

	if (!nm_auth_is_subject_in_acl_set_error (NM_CONNECTION (connection),
	                                          subject,
	                                          NM_SETTINGS_ERROR,
	                                          NM_SETTINGS_ERROR_PERMISSION_DENIED,
	                                          &error))
		goto error;

	g_dbus_method_invocation_return_value (invocation,
	                                       g_variant_new ("(o)",
	                                                      nm_dbus_object_get_path (NM_DBUS_OBJECT (connection))));
	return;

error:
	g_dbus_method_invocation_take_error (invocation, error);
}

static void
_clear_connections_cached_list (NMSettingsPrivate *priv)
{
	if (!priv->connections_cached_list)
		return;

	nm_assert (priv->connections_len == NM_PTRARRAY_LEN (priv->connections_cached_list));

#if NM_MORE_ASSERTS
	/* set the pointer to a bogus value. This makes it more apparent
	 * if somebody has a reference to the cached list and still uses
	 * it. That is a bug, this code just tries to make it blow up
	 * more eagerly. */
	memset (priv->connections_cached_list,
	        0xdeaddead,
	        sizeof (NMSettingsConnection *) * (priv->connections_len + 1));
#endif
	nm_clear_g_free (&priv->connections_cached_list);
}

/**
 * nm_settings_get_connections:
 * @self: the #NMSettings
 * @out_len: (out): (allow-none): returns the number of returned
 *   connections.
 *
 * Returns: (transfer-none): a list of NMSettingsConnections. The list is
 * unsorted and NULL terminated. The result is never %NULL, in case of no
 * connections, it returns an empty list.
 * The returned list is cached internally, only valid until the next
 * NMSettings operation.
 */
NMSettingsConnection *const*
nm_settings_get_connections (NMSettings *self, guint *out_len)
{
	NMSettingsPrivate *priv;
	NMSettingsConnection **v;
	NMSettingsConnection *con;
	guint i;

	g_return_val_if_fail (NM_IS_SETTINGS (self), NULL);

	priv = NM_SETTINGS_GET_PRIVATE (self);

	nm_assert (priv->connections_len == c_list_length (&priv->connections_lst_head));

	if (G_UNLIKELY (!priv->connections_cached_list)) {
		v = g_new (NMSettingsConnection *, priv->connections_len + 1);

		i = 0;
		c_list_for_each_entry (con, &priv->connections_lst_head, _connections_lst) {
			nm_assert (i < priv->connections_len);
			v[i++] = con;
		}
		nm_assert (i == priv->connections_len);
		v[i] = NULL;

		priv->connections_cached_list = v;
	}

	NM_SET_OUT (out_len, priv->connections_len);
	return priv->connections_cached_list;
}

/**
 * nm_settings_get_connections_clone:
 * @self: the #NMSetting
 * @out_len: (allow-none): optional output argument
 * @func: caller-supplied function for filtering connections
 * @func_data: caller-supplied data passed to @func
 * @sort_compare_func: (allow-none): optional function pointer for
 *   sorting the returned list.
 * @sort_data: user data for @sort_compare_func.
 *
 * Returns: (transfer container) (element-type NMSettingsConnection):
 *   an NULL terminated array of #NMSettingsConnection objects that were
 *   filtered by @func (or all connections if no filter was specified).
 *   The order is arbitrary.
 *   Caller is responsible for freeing the returned array with free(),
 *   the contained values do not need to be unrefed.
 */
NMSettingsConnection **
nm_settings_get_connections_clone (NMSettings *self,
                                   guint *out_len,
                                   NMSettingsConnectionFilterFunc func,
                                   gpointer func_data,
                                   GCompareDataFunc sort_compare_func,
                                   gpointer sort_data)
{
	NMSettingsConnection *const*list_cached;
	NMSettingsConnection **list;
	guint len, i, j;

	g_return_val_if_fail (NM_IS_SETTINGS (self), NULL);

	list_cached = nm_settings_get_connections (self, &len);

#if NM_MORE_ASSERTS
	nm_assert (list_cached);
	for (i = 0; i < len; i++)
		nm_assert (NM_IS_SETTINGS_CONNECTION (list_cached[i]));
	nm_assert (!list_cached[i]);
#endif

	list = g_new (NMSettingsConnection *, ((gsize) len + 1));
	if (func) {
		for (i = 0, j = 0; i < len; i++) {
			if (func (self, list_cached[i], func_data))
				list[j++] = list_cached[i];
		}
		list[j] = NULL;
		len = j;
	} else
		memcpy (list, list_cached, sizeof (list[0]) * ((gsize) len + 1));

	if (   len > 1
	    && sort_compare_func) {
		g_qsort_with_data (list, len, sizeof (NMSettingsConnection *),
		                   sort_compare_func, sort_data);
	}
	NM_SET_OUT (out_len, len);
	return list;
}

NMSettingsConnection *
nm_settings_get_connection_by_path (NMSettings *self, const char *path)
{
	NMSettingsPrivate *priv;
	NMSettingsConnection *connection;

	g_return_val_if_fail (NM_IS_SETTINGS (self), NULL);
	g_return_val_if_fail (path, NULL);

	priv = NM_SETTINGS_GET_PRIVATE (self);

	connection = (NMSettingsConnection *) byx_dbus_manager_lookup_object (nm_dbus_object_get_manager (NM_DBUS_OBJECT (self)),
	                                                                     path);
	if (   !connection
	    || !NM_IS_SETTINGS_CONNECTION (connection))
		return NULL;

	nm_assert (c_list_contains (&priv->connections_lst_head, &connection->_connections_lst));
	return connection;
}

gboolean
nm_settings_has_connection (NMSettings *self, NMSettingsConnection *connection)
{
	gboolean has;

	g_return_val_if_fail (NM_IS_SETTINGS (self), FALSE);
	g_return_val_if_fail (NM_IS_SETTINGS_CONNECTION (connection), FALSE);

	has = !c_list_is_empty (&connection->_connections_lst);

	nm_assert (has == nm_c_list_contains_entry (&NM_SETTINGS_GET_PRIVATE (self)->connections_lst_head,
                                                connection,
                                                _connections_lst));
	nm_assert (({
		NMSettingsConnection *candidate = NULL;
		const char *path;

		path = nm_dbus_object_get_path (NM_DBUS_OBJECT (connection));
		if (path)
			candidate = nm_settings_get_connection_by_path (self, path);

		(has == (connection == candidate));
	}));

	return has;
}

const GSList *
nm_settings_get_unmanaged_specs (NMSettings *self)
{
	NMSettingsPrivate *priv = NM_SETTINGS_GET_PRIVATE (self);

	return priv->unmanaged_specs;
}

static NMSettingsPlugin *
get_plugin (NMSettings *self, gboolean has_add_connection)
{
	NMSettingsPrivate *priv = NM_SETTINGS_GET_PRIVATE (self);
	GSList *iter;

	g_return_val_if_fail (self != NULL, NULL);

	/* Do any of the plugins support the given capability? */
	for (iter = priv->plugins; iter; iter = iter->next) {
		NMSettingsPlugin *plugin = NM_SETTINGS_PLUGIN (iter->data);

		if (!has_add_connection)
			return plugin;

		if (NM_SETTINGS_PLUGIN_GET_INTERFACE (iter->data)->add_connection != NULL)
			return plugin;
	}

	return NULL;
}

static gboolean
find_spec (GSList *spec_list, const char *spec)
{
	GSList *iter;

	for (iter = spec_list; iter; iter = g_slist_next (iter)) {
		if (!strcmp ((const char *) iter->data, spec))
			return TRUE;
	}
	return FALSE;
}

static void
update_specs (NMSettings *self, GSList **specs_ptr,
              GSList * (*get_specs_func) (NMSettingsPlugin *))
{
	NMSettingsPrivate *priv = NM_SETTINGS_GET_PRIVATE (self);
	GSList *iter;

	g_slist_free_full (*specs_ptr, g_free);
	*specs_ptr = NULL;

	for (iter = priv->plugins; iter; iter = g_slist_next (iter)) {
		GSList *specs, *specs_iter;

		specs = get_specs_func (NM_SETTINGS_PLUGIN (iter->data));
		for (specs_iter = specs; specs_iter; specs_iter = specs_iter->next) {
			if (!find_spec (*specs_ptr, (const char *) specs_iter->data)) {
				*specs_ptr = g_slist_prepend (*specs_ptr, specs_iter->data);
			} else
				g_free (specs_iter->data);
		}

		g_slist_free (specs);
	}
}

static void
unmanaged_specs_changed (NMSettingsPlugin *config,
                         gpointer user_data)
{
	NMSettings *self = NM_SETTINGS (user_data);
	NMSettingsPrivate *priv = NM_SETTINGS_GET_PRIVATE (self);

	update_specs (self, &priv->unmanaged_specs,
	              nm_settings_plugin_get_unmanaged_specs);
	_notify (self, PROP_UNMANAGED_SPECS);
}

static void
unrecognized_specs_changed (NMSettingsPlugin *config,
                               gpointer user_data)
{
	NMSettings *self = NM_SETTINGS (user_data);
	NMSettingsPrivate *priv = NM_SETTINGS_GET_PRIVATE (self);

	update_specs (self, &priv->unrecognized_specs,
	              nm_settings_plugin_get_unrecognized_specs);
}

static gboolean
add_plugin (NMSettings *self, NMSettingsPlugin *plugin)
{
	NMSettingsPrivate *priv;
	const char *path;

	g_return_val_if_fail (NM_IS_SETTINGS (self), FALSE);
	g_return_val_if_fail (NM_IS_SETTINGS_PLUGIN (plugin), FALSE);

	priv = NM_SETTINGS_GET_PRIVATE (self);

	if (g_slist_find (priv->plugins, plugin)) {
		/* don't add duplicates. */
		return FALSE;
	}

	priv->plugins = g_slist_append (priv->plugins, g_object_ref (plugin));
	nm_settings_plugin_init (plugin);


	path = g_object_get_qdata (G_OBJECT (plugin), plugin_module_path_quark ());

	_LOGI ("Loaded settings plugin: %s (%s)", G_OBJECT_TYPE_NAME (plugin), path ?: "internal");

	return TRUE;
}

static gboolean
plugin_loaded (GSList *list, const char *path)
{
	GSList *iter;

	g_return_val_if_fail (path != NULL, TRUE);

	for (iter = list; iter; iter = g_slist_next (iter)) {
		const char *list_path = g_object_get_qdata (G_OBJECT (iter->data),
		                                            plugin_module_path_quark ());

		if (g_strcmp0 (path, list_path) == 0)
			return TRUE;
	}

	return FALSE;
}

static gboolean
load_plugin (NMSettings *self, GSList **list, const char *pname, GError **error)
{
	gs_free char *full_name = NULL;
	gs_free char *path = NULL;
	gs_unref_object GObject *obj = NULL;
	GModule *plugin;
	GObject * (*factory_func) (void);
	struct stat st;
	int errsv;

	full_name = g_strdup_printf ("nm-settings-plugin-%s", pname);
	path = g_module_build_path (NMPLUGINDIR, full_name);

	if (plugin_loaded (*list, path))
		return TRUE;

	if (stat (path, &st) != 0) {
		errsv = errno;
		_LOGW ("could not load plugin '%s' from file '%s': %s", pname, path, strerror (errsv));
		return TRUE;
	}
	if (!S_ISREG (st.st_mode)) {
		_LOGW ("could not load plugin '%s' from file '%s': not a file", pname, path);
		return TRUE;
	}
	if (st.st_uid != 0) {
		_LOGW ("could not load plugin '%s' from file '%s': file must be owned by root", pname, path);
		return TRUE;
	}
	if (st.st_mode & (S_IWGRP | S_IWOTH | S_ISUID)) {
		_LOGW ("could not load plugin '%s' from file '%s': invalid file permissions", pname, path);
		return TRUE;
	}

	plugin = g_module_open (path, G_MODULE_BIND_LOCAL);
	if (!plugin) {
		_LOGW ("could not load plugin '%s' from file '%s': %s",
		     pname, path, g_module_error ());
		return TRUE;
	}

	/* errors after this point are fatal, because we loaded the shared library already. */

	if (!g_module_symbol (plugin, "nm_settings_plugin_factory", (gpointer) (&factory_func))) {
		g_set_error (error, NM_SETTINGS_ERROR, NM_SETTINGS_ERROR_FAILED,
		             "Could not find plugin '%s' factory function.",
		             pname);
		g_module_close (plugin);
		return FALSE;
	}

	/* after accessing the plugin we cannot unload it anymore, because the glib
	 * types cannot be properly unregistered. */
	g_module_make_resident (plugin);

	obj = (*factory_func) ();
	if (!obj || !NM_IS_SETTINGS_PLUGIN (obj)) {
		g_set_error (error, NM_SETTINGS_ERROR, NM_SETTINGS_ERROR_FAILED,
		             "Plugin '%s' returned invalid system config object.",
		             pname);
		return FALSE;
	}

	g_object_set_qdata_full (obj, plugin_module_path_quark (), path, g_free);
	path = NULL;
	if (add_plugin (self, NM_SETTINGS_PLUGIN (obj)))
		*list = g_slist_append (*list, g_steal_pointer (&obj));

	return TRUE;
}

static void
add_keyfile_plugin (NMSettings *self)
{
	gs_unref_object NMSKeyfilePlugin *keyfile_plugin = NULL;

	keyfile_plugin = nms_keyfile_plugin_new ();
	if (!add_plugin (self, NM_SETTINGS_PLUGIN (keyfile_plugin)))
		g_return_if_reached ();
}

static gboolean
load_plugins (NMSettings *self, const char **plugins, GError **error)
{
	GSList *list = NULL;
	const char **iter;
	gboolean keyfile_added = FALSE;
	gboolean success = TRUE;
	gboolean add_ibft = FALSE;
	gboolean has_no_ibft;
	gssize idx_no_ibft, idx_ibft;

	idx_ibft    = byx_utils_strv_find_first ((char **) plugins, -1, "ibft");
	idx_no_ibft = byx_utils_strv_find_first ((char **) plugins, -1, "no-ibft");
	has_no_ibft = idx_no_ibft >= 0 && idx_no_ibft > idx_ibft;

	for (iter = plugins; iter && *iter; iter++) {
		const char *pname = *iter;

		if (!*pname || strchr (pname, '/')) {
			_LOGW ("ignore invalid plugin \"%s\"", pname);
			continue;
		}

		if (NM_IN_STRSET (pname, "ifcfg-suse", "ifnet")) {
			_LOGW ("skipping deprecated plugin %s", pname);
			continue;
		}

		if (!strcmp (pname, "no-ibft"))
			continue;
		if (has_no_ibft && !strcmp (pname, "ibft"))
			continue;

		/* keyfile plugin is built-in now */
		if (strcmp (pname, "keyfile") == 0) {
			if (!keyfile_added) {
				add_keyfile_plugin (self);
				keyfile_added = TRUE;
			}
			continue;
		}

		if (byx_utils_strv_find_first ((char **) plugins,
		                              iter - plugins,
		                              pname) >= 0) {
			/* the plugin is already mentioned in the list previously.
			 * Don't load a duplicate. */
			continue;
		}

		success = load_plugin (self, &list, pname, error);
		if (!success)
			break;

		if (add_ibft && !strcmp (pname, "ifcfg-rh")) {
			/* The plugin ibft is not explicitly mentioned but we just enabled "ifcfg-rh".
			 * Enable "ibft" by default after "ifcfg-rh". */
			pname = "ibft";
			add_ibft = FALSE;

			success = load_plugin (self, &list, "ibft", error);
			if (!success)
				break;
		}
	}

	/* If keyfile plugin was not among configured plugins, add it as the last one */
	if (!keyfile_added)
		add_keyfile_plugin (self);

	g_slist_free_full (list, g_object_unref);

	return success;
}

static void
connection_updated (NMSettingsConnection *connection, gboolean by_user, gpointer user_data)
{
	g_signal_emit (NM_SETTINGS (user_data),
	               signals[CONNECTION_UPDATED],
	               0,
	               connection,
	               by_user);
}

static void
connection_flags_changed (NMSettingsConnection *connection,
                          gpointer user_data)
{
	g_signal_emit (NM_SETTINGS (user_data),
	               signals[CONNECTION_FLAGS_CHANGED],
	               0,
	               connection);
}

static void
connection_removed (NMSettingsConnection *connection, gpointer user_data)
{
	NMSettings *self = NM_SETTINGS (user_data);
	NMSettingsPrivate *priv = NM_SETTINGS_GET_PRIVATE (self);
	NMDevice *device;

	g_return_if_fail (NM_IS_SETTINGS_CONNECTION (connection));
	g_return_if_fail (!c_list_is_empty (&connection->_connections_lst));
	nm_assert (c_list_contains (&priv->connections_lst_head, &connection->_connections_lst));

	/* When the default wired connection is removed (either deleted or saved to
	 * a new persistent connection by a plugin), write the MAC address of the
	 * wired device to the config file and don't create a new default wired
	 * connection for that device again.
	 */
	device = g_object_get_qdata (G_OBJECT (connection), _default_wired_device_quark ());
	if (device)
		default_wired_clear_tag (self, device, connection, TRUE);

	/* Disconnect signal handlers, as plugins might still keep references
	 * to the connection (and thus the signal handlers would still be live)
	 * even after NMSettings has dropped all its references.
	 */

	g_signal_handlers_disconnect_by_func (connection, G_CALLBACK (connection_removed), self);
	g_signal_handlers_disconnect_by_func (connection, G_CALLBACK (connection_updated), self);
	g_signal_handlers_disconnect_by_func (connection, G_CALLBACK (connection_flags_changed), self);
	if (!priv->startup_complete)
		g_signal_handlers_disconnect_by_func (connection, G_CALLBACK (connection_ready_changed), self);

	/* Forget about the connection internally */
	_clear_connections_cached_list (priv);
	priv->connections_len--;
	c_list_unlink (&connection->_connections_lst);

	if (priv->connections_loaded) {
		_notify (self, PROP_CONNECTIONS);

		nm_dbus_object_emit_signal (NM_DBUS_OBJECT (self),
		                            &interface_info_settings,
		                            &signal_info_connection_removed,
		                            "(o)",
		                            nm_dbus_object_get_path (NM_DBUS_OBJECT (connection)));
	}

	nm_dbus_object_unexport (NM_DBUS_OBJECT (connection));

	if (priv->connections_loaded)
		g_signal_emit (self, signals[CONNECTION_REMOVED], 0, connection);

	g_object_unref (connection);

	check_startup_complete (self);

	g_object_unref (self);       /* Balanced by a ref in claim_connection() */
}

#define NM_DBUS_SERVICE_OPENCONNECT    "org.freedesktop.NetworkManager.openconnect"
#define NM_OPENCONNECT_KEY_GATEWAY "gateway"
#define NM_OPENCONNECT_KEY_COOKIE "cookie"
#define NM_OPENCONNECT_KEY_GWCERT "gwcert"
#define NM_OPENCONNECT_KEY_XMLCONFIG "xmlconfig"
#define NM_OPENCONNECT_KEY_LASTHOST "lasthost"
#define NM_OPENCONNECT_KEY_AUTOCONNECT "autoconnect"
#define NM_OPENCONNECT_KEY_CERTSIGS "certsigs"

static void
openconnect_migrate_hack (NMConnection *connection)
{
	NMSettingVpn *s_vpn;
	NMSettingSecretFlags flags = NM_SETTING_SECRET_FLAG_NOT_SAVED;

	/* Huge hack.  There were some openconnect changes that needed to happen
	 * pretty late, too late to get into distros.  Migration has already
	 * happened for many people, and their secret flags are wrong.  But we
	 * don't want to requrie re-migration, so we have to fix it up here. Ugh.
	 */

	s_vpn = nm_connection_get_setting_vpn (connection);
	if (s_vpn == NULL)
		return;

	if (g_strcmp0 (nm_setting_vpn_get_service_type (s_vpn), NM_DBUS_SERVICE_OPENCONNECT) == 0) {
		/* These are different for every login session, and should not be stored */
		nm_setting_set_secret_flags (NM_SETTING (s_vpn), NM_OPENCONNECT_KEY_GATEWAY, flags, NULL);
		nm_setting_set_secret_flags (NM_SETTING (s_vpn), NM_OPENCONNECT_KEY_COOKIE, flags, NULL);
		nm_setting_set_secret_flags (NM_SETTING (s_vpn), NM_OPENCONNECT_KEY_GWCERT, flags, NULL);

		/* These are purely internal data for the auth-dialog, and should be stored */
		flags = 0;
		nm_setting_set_secret_flags (NM_SETTING (s_vpn), NM_OPENCONNECT_KEY_XMLCONFIG, flags, NULL);
		nm_setting_set_secret_flags (NM_SETTING (s_vpn), NM_OPENCONNECT_KEY_LASTHOST, flags, NULL);
		nm_setting_set_secret_flags (NM_SETTING (s_vpn), NM_OPENCONNECT_KEY_AUTOCONNECT, flags, NULL);
		nm_setting_set_secret_flags (NM_SETTING (s_vpn), NM_OPENCONNECT_KEY_CERTSIGS, flags, NULL);
	}
}

static void
claim_connection (NMSettings *self, NMSettingsConnection *connection)
{
	NMSettingsPrivate *priv = NM_SETTINGS_GET_PRIVATE (self);
	GError *error = NULL;
	const char *path;
	NMSettingsConnection *existing;

	g_return_if_fail (NM_IS_SETTINGS_CONNECTION (connection));
	g_return_if_fail (!nm_dbus_object_is_exported (NM_DBUS_OBJECT (connection)));

	/* prevent duplicates */
	if (!c_list_is_empty (&connection->_connections_lst)) {
		nm_assert (c_list_contains (&priv->connections_lst_head, &connection->_connections_lst));
		return;
	}

	if (!nm_connection_normalize (NM_CONNECTION (connection), NULL, NULL, &error)) {
		_LOGW ("plugin provided invalid connection: %s", error->message);
		g_error_free (error);
		return;
	}

	existing = nm_settings_get_connection_by_uuid (self, nm_settings_connection_get_uuid (connection));
	if (existing) {
		/* Cannot add duplicate connections per UUID. Just return without action and
		 * log a warning.
		 *
		 * This means, that plugins must not provide duplicate connections (UUID).
		 * In fact, none of the plugins currently would do that.
		 *
		 * But globaly, over different setting plugins, there could be duplicates
		 * without the individual plugins being aware. Don't handle that at all, just
		 * error out. That should not happen unless the admin misconfigured the system
		 * to create conflicting connections. */
		_LOGW ("plugin provided duplicate connection with UUID %s",
		       nm_settings_connection_get_uuid (connection));
		return;
	}

	/* Read timestamp from look-aside file and put it into the connection's data */
	nm_settings_connection_read_and_fill_timestamp (connection);

	/* Read seen-bssids from look-aside file and put it into the connection's data */
	nm_settings_connection_read_and_fill_seen_bssids (connection);

	/* Ensure its initial visibility is up-to-date */
	nm_settings_connection_recheck_visibility (connection);

	/* Evil openconnect migration hack */
	openconnect_migrate_hack (NM_CONNECTION (connection));

	/* This one unexports the connection, it needs to run late to give the active
	 * connection a chance to deal with its reference to this settings connection. */
	g_signal_connect_after (connection, NM_SETTINGS_CONNECTION_REMOVED,
	                        G_CALLBACK (connection_removed), self);
	g_signal_connect (connection, NM_SETTINGS_CONNECTION_UPDATED_INTERNAL,
	                  G_CALLBACK (connection_updated), self);
	g_signal_connect (connection, NM_SETTINGS_CONNECTION_FLAGS_CHANGED,
	                  G_CALLBACK (connection_flags_changed),
	                  self);
	if (!priv->startup_complete) {
		g_signal_connect (connection, "notify::" NM_SETTINGS_CONNECTION_READY,
		                  G_CALLBACK (connection_ready_changed),
		                  self);
	}

	_clear_connections_cached_list (priv);

	g_object_ref (connection);
	/* FIXME(shutdown): The NMSettings instance can't be disposed
	 * while there is any exported connection. Ideally we should
	 * unexport all connections on NMSettings' disposal, but for now
	 * leak @self on termination when there are connections alive. */
	g_object_ref (self);
	priv->connections_len++;
	c_list_link_tail (&priv->connections_lst_head, &connection->_connections_lst);

	path = nm_dbus_object_export (NM_DBUS_OBJECT (connection));

	byx_utils_log_connection_diff (NM_CONNECTION (connection), NULL, LOGL_DEBUG, LOGD_CORE, "new connection", "++ ",
	                              path);

	/* Only emit the individual connection-added signal after connections
	 * have been initially loaded.
	 */
	if (priv->connections_loaded) {
		nm_dbus_object_emit_signal (NM_DBUS_OBJECT (self),
		                            &interface_info_settings,
		                            &signal_info_new_connection,
		                            "(o)",
		                            nm_dbus_object_get_path (NM_DBUS_OBJECT (connection)));

		g_signal_emit (self, signals[CONNECTION_ADDED], 0, connection);
		_notify (self, PROP_CONNECTIONS);
	}

	nm_settings_connection_added (connection);
}

static gboolean
secrets_filter_cb (NMSetting *setting,
                   const char *secret,
                   NMSettingSecretFlags flags,
                   gpointer user_data)
{
	NMSettingSecretFlags filter_flags = GPOINTER_TO_UINT (user_data);

	/* Returns TRUE to remove the secret */

	/* Can't use bitops with SECRET_FLAG_NONE so handle that specifically */
	if (   (flags == NM_SETTING_SECRET_FLAG_NONE)
	    && (filter_flags == NM_SETTING_SECRET_FLAG_NONE))
		return FALSE;

	/* Otherwise if the secret has at least one of the desired flags keep it */
	return (flags & filter_flags) ? FALSE : TRUE;
}

/**
 * nm_settings_add_connection:
 * @self: the #NMSettings object
 * @connection: the source connection to create a new #NMSettingsConnection from
 * @save_to_disk: %TRUE to save the connection to disk immediately, %FALSE to
 * not save to disk
 * @error: on return, a location to store any errors that may occur
 *
 * Creates a new #NMSettingsConnection for the given source @connection.
 * The returned object is owned by @self and the caller must reference
 * the object to continue using it.
 *
 * Returns: the new #NMSettingsConnection or %NULL
 */
NMSettingsConnection *
nm_settings_add_connection (NMSettings *self,
                            NMConnection *connection,
                            gboolean save_to_disk,
                            GError **error)
{
	NMSettingsPrivate *priv = NM_SETTINGS_GET_PRIVATE (self);
	GSList *iter;
	NMSettingsConnection *added = NULL;
	NMSettingsConnection *candidate = NULL;
	const char *uuid;

	uuid = nm_connection_get_uuid (connection);

	/* Make sure a connection with this UUID doesn't already exist */
	c_list_for_each_entry (candidate, &priv->connections_lst_head, _connections_lst) {
		if (nm_streq0 (uuid, nm_connection_get_uuid (NM_CONNECTION (candidate)))) {
			g_set_error_literal (error,
			                     NM_SETTINGS_ERROR,
			                     NM_SETTINGS_ERROR_UUID_EXISTS,
			                     "A connection with this UUID already exists.");
			return NULL;
		}
	}

	/* 1) plugin writes the NMConnection to disk
	 * 2) plugin creates a new NMSettingsConnection subclass with the settings
	 *     from the NMConnection and returns it to the settings service
	 * 3) settings service exports the new NMSettingsConnection subclass
	 * 4) plugin notices that something on the filesystem has changed
	 * 5) plugin reads the changes and ignores them because they will
	 *     contain the same data as the connection it already knows about
	 */
	for (iter = priv->plugins; iter; iter = g_slist_next (iter)) {
		NMSettingsPlugin *plugin = NM_SETTINGS_PLUGIN (iter->data);
		GError *add_error = NULL;
		gs_unref_object NMConnection *simple = NULL;
		gs_unref_variant GVariant *secrets = NULL;

		/* Make a copy of agent-owned secrets because they won't be present in
		 * the connection returned by plugins, as plugins return only what was
		 * reread from the file. */
		simple = nm_simple_connection_new_clone (connection);
		nm_connection_clear_secrets_with_flags (simple,
		                                        secrets_filter_cb,
		                                        GUINT_TO_POINTER (NM_SETTING_SECRET_FLAG_AGENT_OWNED));
		secrets = nm_connection_to_dbus (simple, NM_CONNECTION_SERIALIZE_ONLY_SECRETS);

		added = nm_settings_plugin_add_connection (plugin, connection, save_to_disk, &add_error);
		if (added) {
			if (secrets)
				nm_connection_update_secrets (NM_CONNECTION (added), NULL, secrets, NULL);
			claim_connection (self, added);
			return added;
		}
		_LOGD ("Failed to add %s/'%s': %s",
		       nm_connection_get_uuid (connection),
		       nm_connection_get_id (connection),
		       add_error->message);
		g_clear_error (&add_error);
	}

	g_set_error_literal (error, NM_SETTINGS_ERROR, NM_SETTINGS_ERROR_FAILED,
	                     "No plugin supported adding this connection");
	return NULL;
}

static void
send_agent_owned_secrets (NMSettings *self,
                          NMSettingsConnection *connection,
                          NMAuthSubject *subject)
{
	NMSettingsPrivate *priv = NM_SETTINGS_GET_PRIVATE (self);
	gs_unref_object NMConnection *for_agent = NULL;

	/* Dupe the connection so we can clear out non-agent-owned secrets,
	 * as agent-owned secrets are the only ones we send back to be saved.
	 * Only send secrets to agents of the same UID that called update too.
	 */
	for_agent = nm_simple_connection_new_clone (NM_CONNECTION (connection));
	nm_connection_clear_secrets_with_flags (for_agent,
	                                        secrets_filter_cb,
	                                        GUINT_TO_POINTER (NM_SETTING_SECRET_FLAG_AGENT_OWNED));
	nm_agent_manager_save_secrets (priv->agent_mgr,
	                               nm_dbus_object_get_path (NM_DBUS_OBJECT (connection)),
	                               for_agent,
	                               subject);
}

static void
pk_add_cb (NMAuthChain *chain,
           GError *chain_error,
           GDBusMethodInvocation *context,
           gpointer user_data)
{
	NMSettings *self = NM_SETTINGS (user_data);
	NMSettingsPrivate *priv = NM_SETTINGS_GET_PRIVATE (self);
	NMAuthCallResult result;
	GError *error = NULL;
	NMConnection *connection = NULL;
	NMSettingsConnection *added = NULL;
	NMSettingsAddCallback callback;
	gpointer callback_data;
	NMAuthSubject *subject;
	const char *perm;
	gboolean save_to_disk;

	g_assert (context);

	priv->auths = g_slist_remove (priv->auths, chain);

	perm = nm_auth_chain_get_data (chain, "perm");
	g_assert (perm);
	result = nm_auth_chain_get_result (chain, perm);

	if (chain_error) {
		error = g_error_new (NM_SETTINGS_ERROR,
		                     NM_SETTINGS_ERROR_FAILED,
		                     "Error checking authorization: %s",
		                     chain_error->message);
	} else if (result != NM_AUTH_CALL_RESULT_YES) {
		error = g_error_new_literal (NM_SETTINGS_ERROR,
		                             NM_SETTINGS_ERROR_PERMISSION_DENIED,
		                             "Insufficient privileges.");
	} else {
		/* Authorized */
		connection = nm_auth_chain_get_data (chain, "connection");
		g_assert (connection);
		save_to_disk = GPOINTER_TO_UINT (nm_auth_chain_get_data (chain, "save-to-disk"));
		added = nm_settings_add_connection (self, connection, save_to_disk, &error);
	}

	callback = nm_auth_chain_get_data (chain, "callback");
	callback_data = nm_auth_chain_get_data (chain, "callback-data");
	subject = nm_auth_chain_get_data (chain, "subject");

	callback (self, added, error, context, subject, callback_data);

	/* Send agent-owned secrets to the agents */
	if (!error && added && nm_settings_has_connection (self, added))
		send_agent_owned_secrets (self, added, subject);

	g_clear_error (&error);
	nm_auth_chain_destroy (chain);
}

/* FIXME: remove if/when kernel supports adhoc wpa */
static gboolean
is_adhoc_wpa (NMConnection *connection)
{
	NMSettingWireless *s_wifi;
	NMSettingWirelessSecurity *s_wsec;
	const char *mode, *key_mgmt;

	/* The kernel doesn't support Ad-Hoc WPA connections well at this time,
	 * and turns them into open networks.  It's been this way since at least
	 * 2.6.30 or so; until that's fixed, disable WPA-protected Ad-Hoc networks.
	 */

	s_wifi = nm_connection_get_setting_wireless (connection);
	if (!s_wifi)
		return FALSE;

	mode = nm_setting_wireless_get_mode (s_wifi);
	if (g_strcmp0 (mode, NM_SETTING_WIRELESS_MODE_ADHOC) != 0)
		return FALSE;

	s_wsec = nm_connection_get_setting_wireless_security (connection);
	if (!s_wsec)
		return FALSE;

	key_mgmt = nm_setting_wireless_security_get_key_mgmt (s_wsec);
	if (g_strcmp0 (key_mgmt, "wpa-none") != 0)
		return FALSE;

	return TRUE;
}

void
nm_settings_add_connection_dbus (NMSettings *self,
                                 NMConnection *connection,
                                 gboolean save_to_disk,
                                 NMAuthSubject *subject,
                                 GDBusMethodInvocation *context,
                                 NMSettingsAddCallback callback,
                                 gpointer user_data)
{
	NMSettingsPrivate *priv = NM_SETTINGS_GET_PRIVATE (self);
	NMSettingConnection *s_con;
	NMAuthChain *chain;
	GError *error = NULL, *tmp_error = NULL;
	const char *perm;

	g_return_if_fail (NM_IS_CONNECTION (connection));
	g_return_if_fail (NM_IS_AUTH_SUBJECT (subject));
	g_return_if_fail (G_IS_DBUS_METHOD_INVOCATION (context));

	/* Connection must be valid, of course */
	if (!nm_connection_verify (connection, &tmp_error)) {
		error = g_error_new (NM_SETTINGS_ERROR,
		                     NM_SETTINGS_ERROR_INVALID_CONNECTION,
		                     "The connection was invalid: %s",
		                     tmp_error->message);
		g_error_free (tmp_error);
		goto done;
	}

	/* The kernel doesn't support Ad-Hoc WPA connections well at this time,
	 * and turns them into open networks.  It's been this way since at least
	 * 2.6.30 or so; until that's fixed, disable WPA-protected Ad-Hoc networks.
	 */
	if (is_adhoc_wpa (connection)) {
		error = g_error_new_literal (NM_SETTINGS_ERROR,
		                             NM_SETTINGS_ERROR_INVALID_CONNECTION,
		                             "WPA Ad-Hoc disabled due to kernel bugs");
		goto done;
	}

	/* Do any of the plugins support adding? */
	if (!get_plugin (self, TRUE)) {
		error = g_error_new_literal (NM_SETTINGS_ERROR,
		                             NM_SETTINGS_ERROR_NOT_SUPPORTED,
		                             "None of the registered plugins support add.");
		goto done;
	}

	if (!nm_auth_is_subject_in_acl_set_error (connection,
	                                          subject,
	                                          NM_SETTINGS_ERROR,
	                                          NM_SETTINGS_ERROR_PERMISSION_DENIED,
	                                          &error))
		goto done;

	/* If the caller is the only user in the connection's permissions, then
	 * we use the 'modify.own' permission instead of 'modify.system'.  If the
	 * request affects more than just the caller, require 'modify.system'.
	 */
	s_con = nm_connection_get_setting_connection (connection);
	g_assert (s_con);
	if (nm_setting_connection_get_num_permissions (s_con) == 1)
		perm = NM_AUTH_PERMISSION_SETTINGS_MODIFY_OWN;
	else
		perm = NM_AUTH_PERMISSION_SETTINGS_MODIFY_SYSTEM;

	/* Validate the user request */
	chain = nm_auth_chain_new_subject (subject, context, pk_add_cb, self);
	if (!chain) {
		error = g_error_new_literal (NM_SETTINGS_ERROR,
		                             NM_SETTINGS_ERROR_PERMISSION_DENIED,
		                             "Unable to authenticate the request.");
		goto done;
	}

	priv->auths = g_slist_append (priv->auths, chain);
	nm_auth_chain_set_data (chain, "perm", (gpointer) perm, NULL);
	nm_auth_chain_set_data (chain, "connection", g_object_ref (connection), g_object_unref);
	nm_auth_chain_set_data (chain, "callback", callback, NULL);
	nm_auth_chain_set_data (chain, "callback-data", user_data, NULL);
	nm_auth_chain_set_data (chain, "subject", g_object_ref (subject), g_object_unref);
	nm_auth_chain_set_data (chain, "save-to-disk", GUINT_TO_POINTER (save_to_disk), NULL);
	nm_auth_chain_add_call (chain, perm, TRUE);
	return;

done:
	nm_assert (error);
	callback (self, NULL, error, context, subject, user_data);
	g_error_free (error);
}

static void
settings_add_connection_add_cb (NMSettings *self,
                                NMSettingsConnection *connection,
                                GError *error,
                                GDBusMethodInvocation *context,
                                NMAuthSubject *subject,
                                gpointer user_data)
{
	if (error) {
		g_dbus_method_invocation_return_gerror (context, error);
		nm_audit_log_connection_op (NM_AUDIT_OP_CONN_ADD, NULL, FALSE, NULL, subject, error->message);
	} else {
		g_dbus_method_invocation_return_value (context,
		                                       g_variant_new ("(o)",
		                                                      nm_dbus_object_get_path (NM_DBUS_OBJECT (connection))));
		nm_audit_log_connection_op (NM_AUDIT_OP_CONN_ADD, connection, TRUE, NULL,
		                            subject, NULL);
	}
}

static void
settings_add_connection_helper (NMSettings *self,
                                GDBusMethodInvocation *context,
                                GVariant *settings,
                                gboolean save_to_disk)
{
	gs_unref_object NMConnection *connection = NULL;
	GError *error = NULL;
	gs_unref_object NMAuthSubject *subject = NULL;

	connection = _nm_simple_connection_new_from_dbus (settings,
	                                                    NM_SETTING_PARSE_FLAGS_STRICT
	                                                  | NM_SETTING_PARSE_FLAGS_NORMALIZE,
	                                                  &error);

	if (   !connection
	    || !nm_connection_verify_secrets (connection, &error)) {
		g_dbus_method_invocation_take_error (context, error);
		return;
	}

	subject = nm_auth_subject_new_unix_process_from_context (context);
	if (!subject) {
		g_dbus_method_invocation_return_error_literal (context,
		                                               NM_SETTINGS_ERROR,
		                                               NM_SETTINGS_ERROR_PERMISSION_DENIED,
		                                               "Unable to determine UID of request.");
		return;
	}

	nm_settings_add_connection_dbus (self,
	                                 connection,
	                                 save_to_disk,
	                                 subject,
	                                 context,
	                                 settings_add_connection_add_cb,
	                                 NULL);
}

static void
impl_settings_add_connection (ByxDBusObject *obj,
                              const ByxDBusInterfaceInfoExtended *interface_info,
                              const NMDBusMethodInfoExtended *method_info,
                              GDBusConnection *connection,
                              const char *sender,
                              GDBusMethodInvocation *invocation,
                              GVariant *parameters)
{
	NMSettings *self = NM_SETTINGS (obj);
	gs_unref_variant GVariant *settings = NULL;

	g_variant_get (parameters, "(@a{sa{sv}})", &settings);
	settings_add_connection_helper (self, invocation, settings, TRUE);
}

static void
impl_settings_add_connection_unsaved (ByxDBusObject *obj,
                                      const ByxDBusInterfaceInfoExtended *interface_info,
                                      const NMDBusMethodInfoExtended *method_info,
                                      GDBusConnection *connection,
                                      const char *sender,
                                      GDBusMethodInvocation *invocation,
                                      GVariant *parameters)
{
	NMSettings *self = NM_SETTINGS (obj);
	gs_unref_variant GVariant *settings = NULL;

	g_variant_get (parameters, "(@a{sa{sv}})", &settings);
	settings_add_connection_helper (self, invocation, settings, FALSE);
}

static void
impl_settings_load_connections (ByxDBusObject *obj,
                                const ByxDBusInterfaceInfoExtended *interface_info,
                                const NMDBusMethodInfoExtended *method_info,
                                GDBusConnection *connection,
                                const char *sender,
                                GDBusMethodInvocation *invocation,
                                GVariant *parameters)
{
	NMSettings *self = NM_SETTINGS (obj);
	NMSettingsPrivate *priv = NM_SETTINGS_GET_PRIVATE (self);
	gs_unref_ptrarray GPtrArray *failures = NULL;
	GSList *iter;
	guint i;
	gs_free const char **filenames = NULL;

	g_variant_get (parameters, "(^a&s)", &filenames);

	/* The permission is already enforced by the D-Bus daemon, but we ensure
	 * that the caller is still alive so that clients are forced to wait and
	 * we'll be able to switch to polkit without breaking behavior.
	 */
	if (!byx_dbus_manager_ensure_uid (nm_dbus_object_get_manager (obj),
	                                 invocation,
	                                 G_MAXULONG,
	                                 NM_SETTINGS_ERROR,
	                                 NM_SETTINGS_ERROR_PERMISSION_DENIED))
		return;

	if (filenames) {
		for (i = 0; filenames[i]; i++) {
			for (iter = priv->plugins; iter; iter = g_slist_next (iter)) {
				NMSettingsPlugin *plugin = NM_SETTINGS_PLUGIN (iter->data);

				if (nm_settings_plugin_load_connection (plugin, filenames[i]))
					break;
			}

			if (!iter) {
				if (!g_path_is_absolute (filenames[i]))
					_LOGW ("connection filename '%s' is not an absolute path", filenames[i]);
				if (!failures)
					failures = g_ptr_array_new ();
				g_ptr_array_add (failures, (char *) filenames[i]);
			}
		}
	}

	if (failures)
		g_ptr_array_add (failures, NULL);

	g_dbus_method_invocation_return_value (invocation,
	                                       g_variant_new ("(b^as)",
	                                                      (gboolean) (!!failures),
	                                                      failures
	                                                        ? (const char **) failures->pdata
	                                                        : NM_PTRARRAY_EMPTY (const char *)));
}

static void
impl_settings_reload_connections (ByxDBusObject *obj,
                                  const ByxDBusInterfaceInfoExtended *interface_info,
                                  const NMDBusMethodInfoExtended *method_info,
                                  GDBusConnection *connection,
                                  const char *sender,
                                  GDBusMethodInvocation *invocation,
                                  GVariant *parameters)
{
	NMSettings *self = NM_SETTINGS (obj);
	NMSettingsPrivate *priv = NM_SETTINGS_GET_PRIVATE (self);
	GSList *iter;

	/* The permission is already enforced by the D-Bus daemon, but we ensure
	 * that the caller is still alive so that clients are forced to wait and
	 * we'll be able to switch to polkit without breaking behavior.
	 */
	if (!byx_dbus_manager_ensure_uid (nm_dbus_object_get_manager (obj),
	                                 invocation,
	                                 G_MAXULONG,
	                                 NM_SETTINGS_ERROR,
	                                 NM_SETTINGS_ERROR_PERMISSION_DENIED))
		return;

	for (iter = priv->plugins; iter; iter = g_slist_next (iter)) {
		NMSettingsPlugin *plugin = NM_SETTINGS_PLUGIN (iter->data);

		nm_settings_plugin_reload_connections (plugin);
	}

	g_dbus_method_invocation_return_value (invocation, g_variant_new ("(b)", TRUE));
}

/*****************************************************************************/

static gboolean
have_connection_for_device (NMSettings *self, NMDevice *device)
{
	NMSettingsPrivate *priv = NM_SETTINGS_GET_PRIVATE (self);
	NMSettingConnection *s_con;
	NMSettingWired *s_wired;
	const char *setting_hwaddr;
	const char *perm_hw_addr;
	NMSettingsConnection *connection;

	g_return_val_if_fail (NM_IS_SETTINGS (self), FALSE);

	perm_hw_addr = nm_device_get_permanent_hw_address (device);

	/* Find a wired connection locked to the given MAC address, if any */
	c_list_for_each_entry (connection, &priv->connections_lst_head, _connections_lst) {
		const char *ctype, *iface;

		if (!nm_device_check_connection_compatible (device, NM_CONNECTION (connection)))
			continue;

		s_con = nm_connection_get_setting_connection (NM_CONNECTION (connection));

		iface = nm_setting_connection_get_interface_name (s_con);
		if (iface && strcmp (iface, nm_device_get_iface (device)) != 0)
			continue;

		ctype = nm_setting_connection_get_connection_type (s_con);
		if (   strcmp (ctype, NM_SETTING_WIRED_SETTING_NAME)
		    && strcmp (ctype, NM_SETTING_PPPOE_SETTING_NAME))
			continue;

		s_wired = nm_connection_get_setting_wired (NM_CONNECTION (connection));

		if (!s_wired && !strcmp (ctype, NM_SETTING_PPPOE_SETTING_NAME)) {
			/* No wired setting; therefore the PPPoE connection applies to any device */
			return TRUE;
		}

		g_assert (s_wired != NULL);

		setting_hwaddr = nm_setting_wired_get_mac_address (s_wired);
		if (setting_hwaddr) {
			/* A connection mac-locked to this device */
			if (   perm_hw_addr
			    && byx_utils_hwaddr_matches (setting_hwaddr, -1, perm_hw_addr, -1))
				return TRUE;
		} else {
			/* A connection that applies to any wired device */
			return TRUE;
		}
	}

	/* See if there's a known non-NetworkManager configuration for the device */
	if (nm_device_spec_match_list (device, priv->unrecognized_specs))
		return TRUE;

	return FALSE;
}

static void
default_wired_connection_updated_by_user_cb (NMSettingsConnection *connection, gboolean by_user, NMSettings *self)
{
	NMDevice *device;

	if (!by_user)
		return;

	/* The connection has been changed by the user, it should no longer be
	 * considered a default wired connection, and should no longer affect
	 * the no-auto-default configuration option.
	 */
	device = g_object_get_qdata (G_OBJECT (connection), _default_wired_device_quark ());
	if (device)
		default_wired_clear_tag (self, device, connection, FALSE);
}

static void
default_wired_clear_tag (NMSettings *self,
                         NMDevice *device,
                         NMSettingsConnection *connection,
                         gboolean add_to_no_auto_default)
{
	g_return_if_fail (NM_IS_SETTINGS (self));
	g_return_if_fail (NM_IS_DEVICE (device));
	g_return_if_fail (NM_IS_CONNECTION (connection));
	g_return_if_fail (device == g_object_get_qdata (G_OBJECT (connection), _default_wired_device_quark ()));
	g_return_if_fail (connection == g_object_get_qdata (G_OBJECT (device), _default_wired_connection_quark ()));

	g_object_set_qdata (G_OBJECT (connection), _default_wired_device_quark (), NULL);
	g_object_set_qdata (G_OBJECT (device), _default_wired_connection_quark (), NULL);

	g_signal_handlers_disconnect_by_func (connection, G_CALLBACK (default_wired_connection_updated_by_user_cb), self);

	if (add_to_no_auto_default)
		nm_config_set_no_auto_default_for_device (NM_SETTINGS_GET_PRIVATE (self)->config, device);
}

static void
device_realized (NMDevice *device, GParamSpec *pspec, NMSettings *self)
{
	NMConnection *connection;
	NMSettingsConnection *added;
	GError *error = NULL;

	if (!nm_device_is_real (device))
		return;

	g_signal_handlers_disconnect_by_func (device,
	                                      G_CALLBACK (device_realized),
	                                      self);

	/* If the device isn't managed or it already has a default wired connection,
	 * ignore it.
	 */
	if (   !nm_device_get_managed (device, FALSE)
	    || g_object_get_qdata (G_OBJECT (device), _default_wired_connection_quark ())
	    || have_connection_for_device (self, device))
		return;

	connection = nm_device_new_default_connection (device);
	if (!connection)
		return;

	/* Add the connection */
	added = nm_settings_add_connection (self, connection, FALSE, &error);
	g_object_unref (connection);

	if (!added) {
		if (!g_error_matches (error, NM_SETTINGS_ERROR, NM_SETTINGS_ERROR_UUID_EXISTS)) {
			_LOGW ("(%s) couldn't create default wired connection: %s",
			       nm_device_get_iface (device),
			       error->message);
		}
		g_clear_error (&error);
		return;
	}

	g_object_set_qdata (G_OBJECT (added), _default_wired_device_quark (), device);
	g_object_set_qdata (G_OBJECT (device), _default_wired_connection_quark (), added);

	g_signal_connect (added, NM_SETTINGS_CONNECTION_UPDATED_INTERNAL,
	                  G_CALLBACK (default_wired_connection_updated_by_user_cb), self);

	_LOGI ("(%s): created default wired connection '%s'",
	       nm_device_get_iface (device),
	       nm_settings_connection_get_id (added));
}

void
nm_settings_device_added (NMSettings *self, NMDevice *device)
{
	if (nm_device_is_real (device))
		device_realized (device, NULL, self);
	else {
		g_signal_connect_after (device, "notify::" NM_DEVICE_REAL,
		                        G_CALLBACK (device_realized),
		                        self);
	}
}

void
nm_settings_device_removed (NMSettings *self, NMDevice *device, gboolean quitting)
{
	NMSettingsConnection *connection;

	g_signal_handlers_disconnect_by_func (device,
	                                      G_CALLBACK (device_realized),
	                                      self);

	connection = g_object_get_qdata (G_OBJECT (device), _default_wired_connection_quark ());
	if (connection) {
		default_wired_clear_tag (self, device, connection, FALSE);

		/* Don't delete the default wired connection on shutdown, so that it
		 * remains up and can be assumed if NM starts again.
		 */
		if (quitting == FALSE)
			nm_settings_connection_delete (connection, NULL);
	}
}

/*****************************************************************************/

gboolean
nm_settings_get_startup_complete (NMSettings *self)
{
	NMSettingsPrivate *priv = NM_SETTINGS_GET_PRIVATE (self);

	return priv->startup_complete;
}

/*****************************************************************************/

gboolean
nm_settings_start (NMSettings *self, GError **error)
{
	NMSettingsPrivate *priv;
	gs_strfreev char **plugins = NULL;

	priv = NM_SETTINGS_GET_PRIVATE (self);

	/* Load the plugins; fail if a plugin is not found. */
	plugins = nm_config_data_get_plugins (nm_config_get_data_orig (priv->config), TRUE);

	if (!load_plugins (self, (const char **) plugins, error))
		return FALSE;

	load_connections (self);
	check_startup_complete (self);

	return TRUE;
}

/*****************************************************************************/

static void
get_property (GObject *object, guint prop_id,
              GValue *value, GParamSpec *pspec)
{
	NMSettings *self = NM_SETTINGS (object);
	NMSettingsPrivate *priv = NM_SETTINGS_GET_PRIVATE (self);
	const GSList *specs, *iter;
	guint i;
	char **strvs;
	const char **strv;

	switch (prop_id) {
	case PROP_UNMANAGED_SPECS:
		specs = nm_settings_get_unmanaged_specs (self);
		strvs = g_new (char *, g_slist_length ((GSList *) specs) + 1);
		i = 0;
		for (iter = specs; iter; iter = iter->next)
			strvs[i++] = g_strdup (iter->data);
		strvs[i] = NULL;
		g_value_take_boxed (value, strvs);
		break;
	case PROP_CAN_MODIFY:
		g_value_set_boolean (value, !!get_plugin (self, TRUE));
		break;
	case PROP_CONNECTIONS:
		if (priv->connections_loaded) {
			strv = nm_dbus_utils_get_paths_for_clist (&priv->connections_lst_head,
			                                          priv->connections_len,
			                                          G_STRUCT_OFFSET (NMSettingsConnection, _connections_lst),
			                                          TRUE);
			g_value_take_boxed (value, byx_utils_strv_make_deep_copied (strv));
		} else
			g_value_set_boxed (value, NULL);
		break;
	case PROP_STARTUP_COMPLETE:
		g_value_set_boolean (value, nm_settings_get_startup_complete (self));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

/*****************************************************************************/

static void
nm_settings_init (NMSettings *self)
{
	NMSettingsPrivate *priv = NM_SETTINGS_GET_PRIVATE (self);

	c_list_init (&priv->connections_lst_head);

	priv->agent_mgr = g_object_ref (nm_agent_manager_get ());
	priv->config = g_object_ref (nm_config_get ());
}

NMSettings *
nm_settings_new (void)
{
	return g_object_new (NM_TYPE_SETTINGS, NULL);
}

static void
dispose (GObject *object)
{
	NMSettings *self = NM_SETTINGS (object);
	NMSettingsPrivate *priv = NM_SETTINGS_GET_PRIVATE (self);

	g_slist_free_full (priv->auths, (GDestroyNotify) nm_auth_chain_destroy);
	priv->auths = NULL;

	G_OBJECT_CLASS (nm_settings_parent_class)->dispose (object);
}

static void
finalize (GObject *object)
{
	NMSettings *self = NM_SETTINGS (object);
	NMSettingsPrivate *priv = NM_SETTINGS_GET_PRIVATE (self);

	_clear_connections_cached_list (priv);

	nm_assert (c_list_is_empty (&priv->connections_lst_head));

	g_slist_free_full (priv->unmanaged_specs, g_free);
	g_slist_free_full (priv->unrecognized_specs, g_free);

	g_slist_free_full (priv->plugins, g_object_unref);

	g_clear_object (&priv->agent_mgr);

	g_clear_object (&priv->config);

	G_OBJECT_CLASS (nm_settings_parent_class)->finalize (object);
}

static const GDBusSignalInfo signal_info_new_connection = NM_DEFINE_GDBUS_SIGNAL_INFO_INIT (
	"NewConnection",
	.args = NM_DEFINE_GDBUS_ARG_INFOS (
		NM_DEFINE_GDBUS_ARG_INFO ("connection", "o"),
	),
);

static const GDBusSignalInfo signal_info_connection_removed = NM_DEFINE_GDBUS_SIGNAL_INFO_INIT (
	"ConnectionRemoved",
	.args = NM_DEFINE_GDBUS_ARG_INFOS (
		NM_DEFINE_GDBUS_ARG_INFO ("connection", "o"),
	),
);

static const ByxDBusInterfaceInfoExtended interface_info_settings = {
	.parent = NM_DEFINE_GDBUS_INTERFACE_INFO_INIT (
		NM_DBUS_INTERFACE_SETTINGS,
		.methods = NM_DEFINE_GDBUS_METHOD_INFOS (
			NM_DEFINE_DBUS_METHOD_INFO_EXTENDED (
				NM_DEFINE_GDBUS_METHOD_INFO_INIT (
					"ListConnections",
					.out_args = NM_DEFINE_GDBUS_ARG_INFOS (
						NM_DEFINE_GDBUS_ARG_INFO ("connections", "ao"),
					),
				),
				.handle = impl_settings_list_connections,
			),
			NM_DEFINE_DBUS_METHOD_INFO_EXTENDED (
				NM_DEFINE_GDBUS_METHOD_INFO_INIT (
					"GetConnectionByUuid",
					.in_args = NM_DEFINE_GDBUS_ARG_INFOS (
						NM_DEFINE_GDBUS_ARG_INFO ("uuid", "s"),
					),
					.out_args = NM_DEFINE_GDBUS_ARG_INFOS (
						NM_DEFINE_GDBUS_ARG_INFO ("connection", "o"),
					),
				),
				.handle = impl_settings_get_connection_by_uuid,
			),
			NM_DEFINE_DBUS_METHOD_INFO_EXTENDED (
				NM_DEFINE_GDBUS_METHOD_INFO_INIT (
					"AddConnection",
					.in_args = NM_DEFINE_GDBUS_ARG_INFOS (
						NM_DEFINE_GDBUS_ARG_INFO ("connection", "a{sa{sv}}"),
					),
					.out_args = NM_DEFINE_GDBUS_ARG_INFOS (
						NM_DEFINE_GDBUS_ARG_INFO ("path", "o"),
					),
				),
				.handle = impl_settings_add_connection,
			),
			NM_DEFINE_DBUS_METHOD_INFO_EXTENDED (
				NM_DEFINE_GDBUS_METHOD_INFO_INIT (
					"AddConnectionUnsaved",
					.in_args = NM_DEFINE_GDBUS_ARG_INFOS (
						NM_DEFINE_GDBUS_ARG_INFO ("connection", "a{sa{sv}}"),
					),
					.out_args = NM_DEFINE_GDBUS_ARG_INFOS (
						NM_DEFINE_GDBUS_ARG_INFO ("path", "o"),
					),
				),
				.handle = impl_settings_add_connection_unsaved,
			),
			NM_DEFINE_DBUS_METHOD_INFO_EXTENDED (
				NM_DEFINE_GDBUS_METHOD_INFO_INIT (
					"LoadConnections",
					.in_args = NM_DEFINE_GDBUS_ARG_INFOS (
						NM_DEFINE_GDBUS_ARG_INFO ("filenames", "as"),
					),
					.out_args = NM_DEFINE_GDBUS_ARG_INFOS (
						NM_DEFINE_GDBUS_ARG_INFO ("status", "b"),
						NM_DEFINE_GDBUS_ARG_INFO ("failures", "as"),
					),
				),
				.handle = impl_settings_load_connections,
			),
			NM_DEFINE_DBUS_METHOD_INFO_EXTENDED (
				NM_DEFINE_GDBUS_METHOD_INFO_INIT (
					"ReloadConnections",
					.out_args = NM_DEFINE_GDBUS_ARG_INFOS (
						NM_DEFINE_GDBUS_ARG_INFO ("status", "b"),
					),
				),
				.handle = impl_settings_reload_connections,
			),
		),
		.signals = NM_DEFINE_GDBUS_SIGNAL_INFOS (
			&nm_signal_info_property_changed_legacy,
			&signal_info_new_connection,
			&signal_info_connection_removed,
		),
		.properties = NM_DEFINE_GDBUS_PROPERTY_INFOS (
			NM_DEFINE_DBUS_PROPERTY_INFO_EXTENDED_READABLE_L ("Connections", "ao", NM_SETTINGS_CONNECTIONS),
			NM_DEFINE_DBUS_PROPERTY_INFO_EXTENDED_READABLE_L ("CanModify",   "b",  NM_SETTINGS_CAN_MODIFY),
		),
	),
	.legacy_property_changed = TRUE,
};

static void
nm_settings_class_init (NMSettingsClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);
	ByxDBusObjectClass *dbus_object_class = NM_DBUS_OBJECT_CLASS (class);

	dbus_object_class->export_path = NM_DBUS_EXPORT_PATH_STATIC (NM_DBUS_PATH_SETTINGS);
	dbus_object_class->interface_infos = NM_DBUS_INTERFACE_INFOS (&interface_info_settings);

	object_class->get_property = get_property;
	object_class->dispose = dispose;
	object_class->finalize = finalize;

	obj_properties[PROP_UNMANAGED_SPECS] =
	    g_param_spec_boxed (NM_SETTINGS_UNMANAGED_SPECS, "", "",
	                        G_TYPE_STRV,
	                        G_PARAM_READABLE |
	                        G_PARAM_STATIC_STRINGS);

	obj_properties[PROP_CAN_MODIFY] =
	    g_param_spec_boolean (NM_SETTINGS_CAN_MODIFY, "", "",
	                          FALSE,
	                          G_PARAM_READABLE |
	                          G_PARAM_STATIC_STRINGS);

	obj_properties[PROP_CONNECTIONS] =
	    g_param_spec_boxed (NM_SETTINGS_CONNECTIONS, "", "",
	                        G_TYPE_STRV,
	                        G_PARAM_READABLE |
	                        G_PARAM_STATIC_STRINGS);

	obj_properties[PROP_STARTUP_COMPLETE] =
	    g_param_spec_boolean (NM_SETTINGS_STARTUP_COMPLETE, "", "",
	                          FALSE,
	                          G_PARAM_READABLE |
	                          G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties (object_class, _PROPERTY_ENUMS_LAST, obj_properties);

	signals[CONNECTION_ADDED] =
	    g_signal_new (NM_SETTINGS_SIGNAL_CONNECTION_ADDED,
	                  G_OBJECT_CLASS_TYPE (object_class),
	                  G_SIGNAL_RUN_FIRST,
	                  0, NULL, NULL,
	                  g_cclosure_marshal_VOID__OBJECT,
	                  G_TYPE_NONE, 1, NM_TYPE_SETTINGS_CONNECTION);

	signals[CONNECTION_UPDATED] =
	    g_signal_new (NM_SETTINGS_SIGNAL_CONNECTION_UPDATED,
	                  G_OBJECT_CLASS_TYPE (object_class),
	                  G_SIGNAL_RUN_FIRST,
	                  0, NULL, NULL,
	                  NULL,
	                  G_TYPE_NONE, 2, NM_TYPE_SETTINGS_CONNECTION, G_TYPE_BOOLEAN);

	signals[CONNECTION_REMOVED] =
	    g_signal_new (NM_SETTINGS_SIGNAL_CONNECTION_REMOVED,
	                  G_OBJECT_CLASS_TYPE (object_class),
	                  G_SIGNAL_RUN_FIRST,
	                  0, NULL, NULL,
	                  g_cclosure_marshal_VOID__OBJECT,
	                  G_TYPE_NONE, 1, NM_TYPE_SETTINGS_CONNECTION);

	signals[CONNECTION_FLAGS_CHANGED] =
	    g_signal_new (NM_SETTINGS_SIGNAL_CONNECTION_FLAGS_CHANGED,
	                  G_OBJECT_CLASS_TYPE (object_class),
	                  G_SIGNAL_RUN_FIRST,
	                  0, NULL, NULL,
	                  g_cclosure_marshal_VOID__OBJECT,
	                  G_TYPE_NONE, 1, NM_TYPE_SETTINGS_CONNECTION);
}
