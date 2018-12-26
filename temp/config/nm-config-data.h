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
 * Copyright (C) 2014 Red Hat, Inc.
 */

#ifndef BYX_CONFIG_DATA_H
#define BYX_CONFIG_DATA_H

#define BYX_TYPE_CONFIG_DATA            (byx_config_data_get_type ())
#define BYX_CONFIG_DATA(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BYX_TYPE_CONFIG_DATA, ByxConfigData))
#define BYX_CONFIG_DATA_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  BYX_TYPE_CONFIG_DATA, ByxConfigDataClass))
#define BYX_IS_CONFIG_DATA(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BYX_TYPE_CONFIG_DATA))
#define BYX_IS_CONFIG_DATA_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  BYX_TYPE_CONFIG_DATA))
#define BYX_CONFIG_DATA_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  BYX_TYPE_CONFIG_DATA, ByxConfigDataClass))

#define BYX_CONFIG_DATA_CONFIG_MAIN_FILE      "config-main-file"
#define BYX_CONFIG_DATA_CONFIG_DESCRIPTION    "config-description"
#define BYX_CONFIG_DATA_KEYFILE_USER          "keyfile-user"
#define BYX_CONFIG_DATA_KEYFILE_INTERN        "keyfile-intern"
#define BYX_CONFIG_DATA_CONNECTIVITY_ENABLED  "connectivity-enabled"
#define BYX_CONFIG_DATA_CONNECTIVITY_URI      "connectivity-uri"
#define BYX_CONFIG_DATA_CONNECTIVITY_INTERVAL "connectivity-interval"
#define BYX_CONFIG_DATA_CONNECTIVITY_RESPONSE "connectivity-response"
#define BYX_CONFIG_DATA_DNS_MODE              "dns"

/* The flags for Reload. Currently these are internal defines,
 * only their numeric value matters and must be stable as
 * they are public API! Also, the enum must fit in uint32. */
enum { /*< skip >*/
	BYX_MANAGER_RELOAD_FLAGS_NONE                            = 0,

	/* reload the configuration from disk */
	BYX_MANAGER_RELOAD_FLAGS_CONF                            = (1LL << 0),

	/* write DNS configuration to resolv.conf */
	BYX_MANAGER_RELOAD_FLAGS_DNS_RC                          = (1LL << 1),

	/* restart the DNS plugin (includes DNS_RC) */
	BYX_MANAGER_RELOAD_FLAGS_DNS_FULL                        = (1LL << 2),

	_BYX_MANAGER_RELOAD_FLAGS_ALL,
	BYX_MANAGER_RELOAD_FLAGS_ALL = ((_BYX_MANAGER_RELOAD_FLAGS_ALL - 1) << 1) - 1,
};

typedef enum { /*< flags >*/
	BYX_CONFIG_GET_VALUE_NONE                   = 0,

	/* use g_key_file_get_value() instead of g_key_file_get_string(). */
	BYX_CONFIG_GET_VALUE_RAW                    = (1LL << 0),

	/* strip whitespaces */
	BYX_CONFIG_GET_VALUE_STRIP                  = (1LL << 1),

	/* if the returned string would be the empty word, return NULL. */
	BYX_CONFIG_GET_VALUE_NO_EMPTY               = (1LL << 2),

	/* special flag to read device spec. You want to use this before passing the
	 * value to nm_match_spec_split(). */
	BYX_CONFIG_GET_VALUE_TYPE_SPEC              = BYX_CONFIG_GET_VALUE_RAW,
} ByxConfigGetValueFlags;

typedef enum { /*< flags >*/
	BYX_CONFIG_CHANGE_NONE                      = 0,

	/**************************************************************************
	 * The external cause which triggered the reload/configuration-change
	 *************************************************************************/

	BYX_CONFIG_CHANGE_CAUSE_SIGHUP              = (1L << 0),
	BYX_CONFIG_CHANGE_CAUSE_SIGUSR1             = (1L << 1),
	BYX_CONFIG_CHANGE_CAUSE_SIGUSR2             = (1L << 2),
	BYX_CONFIG_CHANGE_CAUSE_NO_AUTO_DEFAULT     = (1L << 3),
	BYX_CONFIG_CHANGE_CAUSE_SET_VALUES          = (1L << 4),
	BYX_CONFIG_CHANGE_CAUSE_CONF                = (1L << 5),
	BYX_CONFIG_CHANGE_CAUSE_DNS_RC              = (1L << 6),
	BYX_CONFIG_CHANGE_CAUSE_DNS_FULL            = (1L << 7),

	BYX_CONFIG_CHANGE_CAUSES                    = ((1L << 8) - 1),

	/**************************************************************************
	 * Following flags describe which property of the configuration changed:
	 *************************************************************************/

	/* main-file or config-description changed */
	BYX_CONFIG_CHANGE_CONFIG_FILES              = (1L << 10),

	/* any configuration on disk changed */
	BYX_CONFIG_CHANGE_VALUES                    = (1L << 11),

	/* any user configuration on disk changed (NetworkManager.conf) */
	BYX_CONFIG_CHANGE_VALUES_USER               = (1L << 12),

	/* any internal configuration on disk changed (NetworkManager-intern.conf) */
	BYX_CONFIG_CHANGE_VALUES_INTERN             = (1L << 13),

	/* configuration regarding connectivity changed */
	BYX_CONFIG_CHANGE_CONNECTIVITY              = (1L << 14),

	/* configuration regarding dns-mode changed */
	BYX_CONFIG_CHANGE_DNS_MODE                  = (1L << 16),

	/* configuration regarding rc-manager changed */
	BYX_CONFIG_CHANGE_RC_MANAGER                = (1L << 17),

	/* configuration regarding global dns-config changed */
	BYX_CONFIG_CHANGE_GLOBAL_DNS_CONFIG         = (1L << 18),

} ByxConfigChangeFlags;

typedef struct _ByxConfigDataClass ByxConfigDataClass;

typedef struct _NMGlobalDnsConfig NMGlobalDnsConfig;
typedef struct _NMGlobalDnsDomain NMGlobalDnsDomain;

GType byx_config_data_get_type (void);

ByxConfigData *byx_config_data_new (const char *config_main_file,
                                  const char *config_description,
                                  GKeyFile *keyfile_user,
                                  GKeyFile *keyfile_intern);
ByxConfigData *byx_config_data_new_update_keyfile_intern (const ByxConfigData *base, GKeyFile *keyfile_intern);

ByxConfigChangeFlags byx_config_data_diff (ByxConfigData *old_data, ByxConfigData *new_data);

void byx_config_data_log (const ByxConfigData *self,
                               const char *prefix,
                               const char *key_prefix,
                               /* FILE* */ gpointer print_stream);

const char *byx_config_data_get_config_main_file (const ByxConfigData *config_data);
const char *byx_config_data_get_config_description (const ByxConfigData *config_data);

gboolean byx_config_data_has_group (const ByxConfigData *self, const char *group);
gboolean byx_config_data_has_value (const ByxConfigData *self, const char *group, const char *key, ByxConfigGetValueFlags flags);
char *byx_config_data_get_value (const ByxConfigData *config_data, const char *group, const char *key, ByxConfigGetValueFlags flags);
gint byx_config_data_get_value_boolean (const ByxConfigData *self, const char *group, const char *key, gint default_value);
gint64 byx_config_data_get_value_int64 (const ByxConfigData *self, const char *group, const char *key, guint base, gint64 min, gint64 max, gint64 fallback);

gboolean byx_config_data_get_connectivity_enabled (const ByxConfigData *config_data);
const char *byx_config_data_get_connectivity_uri (const ByxConfigData *config_data);
guint byx_config_data_get_connectivity_interval (const ByxConfigData *config_data);
const char *byx_config_data_get_connectivity_response (const ByxConfigData *config_data);

int byx_config_data_get_autoconnect_retries_default (const ByxConfigData *config_data);

const char *byx_config_data_get_dns_mode (const ByxConfigData *self);
const char *byx_config_data_get_rc_manager (const ByxConfigData *self);

gboolean byx_config_data_get_ignore_carrier (const ByxConfigData *self, NMDevice *device);
gboolean byx_config_data_get_assume_ipv6ll_only (const ByxConfigData *self, NMDevice *device);
int      byx_config_data_get_sriov_num_vfs (const ByxConfigData *self, NMDevice *device);

NMGlobalDnsConfig *byx_config_data_get_global_dns_config (const ByxConfigData *self);

char *byx_config_data_get_connection_default (const ByxConfigData *self,
                                             const char *property,
                                             NMDevice *device);

char *byx_config_data_get_device_config (const ByxConfigData *self,
                                        const char *property,
                                        NMDevice *device,
                                        gboolean *has_match);

char *byx_config_data_get_device_config_by_pllink (const ByxConfigData *self,
                                                  const char *property,
                                                  const NMPlatformLink *pllink,
                                                  const char *match_device_type,
                                                  gboolean *has_match);

gboolean byx_config_data_get_device_config_boolean (const ByxConfigData *self,
                                                   const char *property,
                                                   NMDevice *device,
                                                   gint val_no_match,
                                                   gint val_invalid);

char **byx_config_data_get_groups (const ByxConfigData *self);
char **byx_config_data_get_keys (const ByxConfigData *self, const char *group);
gboolean byx_config_data_is_intern_atomic_group (const ByxConfigData *self, const char *group);

GKeyFile *byx_config_data_clone_keyfile_intern (const ByxConfigData *self);

const char *const *nm_global_dns_config_get_searches (const NMGlobalDnsConfig *dns_config);
const char *const *nm_global_dns_config_get_options (const NMGlobalDnsConfig *dns_config);
guint nm_global_dns_config_get_num_domains (const NMGlobalDnsConfig *dns_config);
NMGlobalDnsDomain *nm_global_dns_config_get_domain (const NMGlobalDnsConfig *dns_config, guint i);
NMGlobalDnsDomain *nm_global_dns_config_lookup_domain (const NMGlobalDnsConfig *dns_config, const char *name);
const char *nm_global_dns_domain_get_name (const NMGlobalDnsDomain *domain);
const char *const *nm_global_dns_domain_get_servers (const NMGlobalDnsDomain *domain);
const char *const *nm_global_dns_domain_get_options (const NMGlobalDnsDomain *domain);
gboolean nm_global_dns_config_is_internal (const NMGlobalDnsConfig *dns_config);
gboolean nm_global_dns_config_is_empty (const NMGlobalDnsConfig *dns_config);
void nm_global_dns_config_update_checksum (const NMGlobalDnsConfig *dns_config, GChecksum *sum);
void nm_global_dns_config_free (NMGlobalDnsConfig *dns_config);

NMGlobalDnsConfig *nm_global_dns_config_from_dbus (const GValue *value, GError **error);
void nm_global_dns_config_to_dbus (const NMGlobalDnsConfig *dns_config, GValue *value);

/* private accessors */
GKeyFile *_byx_config_data_get_keyfile (const ByxConfigData *self);
GKeyFile *_byx_config_data_get_keyfile_user (const ByxConfigData *self);
GKeyFile *_byx_config_data_get_keyfile_intern (const ByxConfigData *self);

#endif /* BYX_CONFIG_DATA_H */

