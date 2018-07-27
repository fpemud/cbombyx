/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* NetworkManager -- Network link manager
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
 * Copyright (C) 2011 Red Hat, Inc.
 * Copyright (C) 2013 Thomas Bechtold <thomasbechtold@jpberlin.de>
 */

#ifndef __BYX_CONFIG_H__
#define __BYX_CONFIG_H__

#include "nm-config-data.h"

#define BYX_TYPE_CONFIG            (byx_config_get_type ())
#define BYX_CONFIG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BYX_TYPE_CONFIG, ByxConfig))
#define BYX_CONFIG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  BYX_TYPE_CONFIG, ByxConfigClass))
#define BYX_IS_CONFIG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BYX_TYPE_CONFIG))
#define BYX_IS_CONFIG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  BYX_TYPE_CONFIG))
#define BYX_CONFIG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  BYX_TYPE_CONFIG, ByxConfigClass))

/* Properties */
#define BYX_CONFIG_CMD_LINE_OPTIONS                  "cmd-line-options"
#define BYX_CONFIG_ATOMIC_SECTION_PREFIXES           "atomic-section-prefixes"

/* Signals */
#define BYX_CONFIG_SIGNAL_CONFIG_CHANGED             "config-changed"

#define BYX_CONFIG_DEFAULT_CONNECTIVITY_INTERVAL 300
#define BYX_CONFIG_DEFAULT_CONNECTIVITY_RESPONSE "NetworkManager is online" /* NOT LOCALIZED */

#define BYX_CONFIG_KEYFILE_LIST_SEPARATOR ','

#define BYX_CONFIG_KEYFILE_GROUPPREFIX_INTERN                ".intern."
#define BYX_CONFIG_KEYFILE_GROUPPREFIX_CONNECTION            "connection"
#define BYX_CONFIG_KEYFILE_GROUPPREFIX_DEVICE                "device"
#define BYX_CONFIG_KEYFILE_GROUPPREFIX_GLOBAL_DNS_DOMAIN     "global-dns-domain-"
#define BYX_CONFIG_KEYFILE_GROUPPREFIX_TEST_APPEND_STRINGLIST ".test-append-stringlist"

#define BYX_CONFIG_KEYFILE_GROUP_MAIN                        "main"
#define BYX_CONFIG_KEYFILE_GROUP_LOGGING                     "logging"
#define BYX_CONFIG_KEYFILE_GROUP_CONNECTIVITY                "connectivity"
#define BYX_CONFIG_KEYFILE_GROUP_GLOBAL_DNS                  "global-dns"
#define BYX_CONFIG_KEYFILE_GROUP_CONFIG                      ".config"

#define BYX_CONFIG_KEYFILE_GROUP_KEYFILE                     "keyfile"

#define BYX_CONFIG_KEYFILE_KEY_MAIN_AUTH_POLKIT              "auth-polkit"
#define BYX_CONFIG_KEYFILE_KEY_MAIN_AUTOCONNECT_RETRIES_DEFAULT "autoconnect-retries-default"
#define BYX_CONFIG_KEYFILE_KEY_MAIN_DHCP                     "dhcp"
#define BYX_CONFIG_KEYFILE_KEY_MAIN_DEBUG                    "debug"
#define BYX_CONFIG_KEYFILE_KEY_MAIN_SLAVES_ORDER             "slaves-order"
#define BYX_CONFIG_KEYFILE_KEY_LOGGING_BACKEND               "backend"
#define BYX_CONFIG_KEYFILE_KEY_CONFIG_ENABLE                 "enable"
#define BYX_CONFIG_KEYFILE_KEY_ATOMIC_SECTION_WAS            ".was"
#define BYX_CONFIG_KEYFILE_KEY_KEYFILE_PATH                  "path"
#define BYX_CONFIG_KEYFILE_KEY_KEYFILE_UNMANAGED_DEVICES     "unmanaged-devices"
#define BYX_CONFIG_KEYFILE_KEY_IFNET_AUTO_REFRESH            "auto_refresh"
#define BYX_CONFIG_KEYFILE_KEY_IFNET_MANAGED                 "managed"
#define BYX_CONFIG_KEYFILE_KEY_IFUPDOWN_MANAGED              "managed"
#define BYX_CONFIG_KEYFILE_KEY_AUDIT                         "audit"

#define BYX_CONFIG_KEYFILE_KEY_DEVICE_MANAGED                "managed"
#define BYX_CONFIG_KEYFILE_KEY_DEVICE_IGNORE_CARRIER         "ignore-carrier"
#define BYX_CONFIG_KEYFILE_KEY_DEVICE_SRIOV_NUM_VFS          "sriov-num-vfs"
#define BYX_CONFIG_KEYFILE_KEY_DEVICE_WIFI_BACKEND           "wifi.backend"
#define BYX_CONFIG_KEYFILE_KEY_DEVICE_WIFI_SCAN_RAND_MAC_ADDRESS "wifi.scan-rand-mac-address"
#define BYX_CONFIG_KEYFILE_KEY_DEVICE_CARRIER_WAIT_TIMEOUT   "carrier-wait-timeout"

#define BYX_CONFIG_KEYFILE_KEYPREFIX_WAS                     ".was."
#define BYX_CONFIG_KEYFILE_KEYPREFIX_SET                     ".set."

#define BYX_CONFIG_KEYFILE_GROUP_INTERN_GLOBAL_DNS \
	BYX_CONFIG_KEYFILE_GROUPPREFIX_INTERN BYX_CONFIG_KEYFILE_GROUP_GLOBAL_DNS
#define BYX_CONFIG_KEYFILE_GROUPPREFIX_INTERN_GLOBAL_DNS_DOMAIN \
	BYX_CONFIG_KEYFILE_GROUPPREFIX_INTERN BYX_CONFIG_KEYFILE_GROUPPREFIX_GLOBAL_DNS_DOMAIN

typedef struct ByxConfigCmdLineOptions ByxConfigCmdLineOptions;

typedef enum {
	BYX_CONFIG_STATE_PROPERTY_NONE,

	/* 1 set-argument: (gboolean enabled) */
	BYX_CONFIG_STATE_PROPERTY_NETWORKING_ENABLED,
	BYX_CONFIG_STATE_PROPERTY_WIFI_ENABLED,
	BYX_CONFIG_STATE_PROPERTY_WWAN_ENABLED,
} ByxConfigRunStatePropertyType;

typedef struct {
	bool net_enabled;
	bool wifi_enabled;
	bool wwan_enabled;

	/* Whether the runstate is modified and not saved to disk. */
	bool dirty;
} ByxConfigState;

typedef struct _ByxConfigClass ByxConfigClass;

GType byx_config_get_type (void);

ByxConfig *byx_config_get (void);

const char *byx_config_change_flags_to_string (ByxConfigChangeFlags flags, char *buf, gsize len);

ByxConfigData *byx_config_get_data (ByxConfig *config);
ByxConfigData *byx_config_get_data_orig (ByxConfig *config);

#define BYX_CONFIG_GET_DATA      (byx_config_get_data (byx_config_get ()))
#define BYX_CONFIG_GET_DATA_ORIG (byx_config_get_data_orig (byx_config_get ()))

gboolean byx_config_get_monitor_connection_files (ByxConfig *config);
const char *byx_config_get_log_level (ByxConfig *config);
const char *byx_config_get_log_domains (ByxConfig *config);
gboolean byx_config_get_configure_and_quit (ByxConfig *config);
gboolean byx_config_get_is_debug (ByxConfig *config);

gboolean byx_config_get_first_start (ByxConfig *config);

void byx_config_set_values (ByxConfig *self,
                           GKeyFile *keyfile_intern_new,
                           gboolean allow_write,
                           gboolean force_rewrite);

/* for main.c only */
ByxConfigCmdLineOptions *byx_config_cmd_line_options_new (gboolean first_start);
void                    byx_config_cmd_line_options_free (ByxConfigCmdLineOptions *cli);
void                    byx_config_cmd_line_options_add_to_entries (ByxConfigCmdLineOptions *cli,
                                                                   GOptionContext *opt_ctx);

gboolean byx_config_get_no_auto_default_for_device (ByxConfig *config, NMDevice *device);
void byx_config_set_no_auto_default_for_device  (ByxConfig *config, NMDevice *device);

ByxConfig *byx_config_new (const ByxConfigCmdLineOptions *cli, char **atomic_section_prefixes, GError **error);
ByxConfig *byx_config_setup (const ByxConfigCmdLineOptions *cli, char **atomic_section_prefixes, GError **error);
void byx_config_reload (ByxConfig *config, ByxConfigChangeFlags reload_flags);

const ByxConfigState *byx_config_state_get (ByxConfig *config);

void _byx_config_state_set (ByxConfig *config,
                           gboolean allow_persist,
                           gboolean force_persist,
                           ...);
#define byx_config_state_set(config, allow_persist, force_persist, ...) \
    _byx_config_state_set (config, allow_persist, force_persist, ##__VA_ARGS__, 0)

gint byx_config_parse_boolean (const char *str, gint default_value);

GKeyFile *byx_config_create_keyfile (void);
gint byx_config_keyfile_get_boolean (const GKeyFile *keyfile,
                                    const char *section,
                                    const char *key,
                                    gint default_value);
gint64 byx_config_keyfile_get_int64 (const GKeyFile *keyfile,
                                    const char *section,
                                    const char *key,
                                    guint base,
                                    gint64 min,
                                    gint64 max,
                                    gint64 fallback);
char *byx_config_keyfile_get_value (const GKeyFile *keyfile,
                                   const char *section,
                                   const char *key,
                                   ByxConfigGetValueFlags flags);
void byx_config_keyfile_set_string_list (GKeyFile *keyfile,
                                        const char *group,
                                        const char *key,
                                        const char *const* strv,
                                        gssize len);
gboolean byx_config_keyfile_has_global_dns_config (GKeyFile *keyfile, gboolean internal);

GSList *byx_config_get_match_spec (const GKeyFile *keyfile, const char *group, const char *key, gboolean *out_has_key);

void _byx_config_sort_groups (char **groups, gsize ngroups);

gboolean byx_config_set_global_dns (ByxConfig *self, NMGlobalDnsConfig *global_dns, GError **error);

void byx_config_set_connectivity_check_enabled (ByxConfig *self, gboolean enabled);

/* internal defines ... */
extern guint _byx_config_match_nm_version;
extern char *_byx_config_match_env;

/*****************************************************************************/

#define BYX_CONFIG_DEVICE_STATE_DIR ""NMRUNDIR"/devices"

#define BYX_CONFIG_DEFAULT_MAIN_AUTH_POLKIT_BOOL     (nm_streq (""BYX_CONFIG_DEFAULT_MAIN_AUTH_POLKIT, "true"))

typedef enum {
	BYX_CONFIG_DEVICE_STATE_MANAGED_TYPE_UNKNOWN   = -1,
	BYX_CONFIG_DEVICE_STATE_MANAGED_TYPE_UNMANAGED = 0,
	BYX_CONFIG_DEVICE_STATE_MANAGED_TYPE_MANAGED   = 1,
} ByxConfigDeviceStateManagedType;

struct _ByxConfigDeviceStateData {
	int ifindex;
	ByxConfigDeviceStateManagedType managed;

	/* a value of zero means that no metric is set. */
	guint32 route_metric_default_aspired;
	guint32 route_metric_default_effective;

	/* the UUID of the last settings-connection active
	 * on the device. */
	const char *connection_uuid;

	const char *perm_hw_addr_fake;

	/* whether the device was nm-owned (0/1) or -1 for
	 * non-software devices. */
	int nm_owned:3;
};

ByxConfigDeviceStateData *byx_config_device_state_load (int ifindex);
GHashTable *byx_config_device_state_load_all (void);

const GHashTable *byx_config_device_state_get_all (ByxConfig *self);
const ByxConfigDeviceStateData *byx_config_device_state_get (ByxConfig *self,
                                                           int ifindex);

/*****************************************************************************/

#endif /* __BYX_CONFIG_H__ */
