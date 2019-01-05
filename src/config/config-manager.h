/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

#ifndef __BYX_CONFIG_MANAGER_H__
#define __BYX_CONFIG_MANAGER_H__

#include "config.h"
#include "run-data.h"
#include "persist-data.h"
#include "connection-run-data.h"
#include "connection-persist-data.h"
#include "service-run-data.h"
#include "service-persist-data.h"

#define BYX_TYPE_CONFIG_MANAGER            (byx_config_manager_get_type ())
#define BYX_CONFIG_MANAGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BYX_TYPE_CONFIG_MANAGER, ByxConfigManager))
#define BYX_CONFIG_MANAGER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  BYX_TYPE_CONFIG_MANAGER, ByxConfigManagerClass))
#define BYX_IS_CONFIG_MANAGER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BYX_TYPE_CONFIG_MANAGER))
#define BYX_IS_CONFIG_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  BYX_TYPE_CONFIG_MANAGER))
#define BYX_CONFIG_MANAGER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  BYX_TYPE_CONFIG_MANAGER, ByxConfigManagerClass))

typedef struct _ByxConfigManager ByxConfigManager;
typedef struct _ByxConfigManagerClass ByxConfigManagerClass;

GType byx_config_manager_get_type (void);

ByxConfigManager *byx_config_manager_setup (int argc, char *argv[], GError **error);

ByxConfigManager *byx_config_manager_get (void);

void byx_config_manager_reload (ByxConfigManager *self);

const ByxConfig *byx_config_manager_get_config(ByxConfigManager *self);

ByxRunData *byx_config_manager_get_run_data (ByxConfigManager *self);
ByxPersistData *byx_config_manager_get_persist_data (ByxConfigManager *self);

ByxConnectionRunData *byx_config_manager_get_connection_run_data (ByxConfigManager *self, const char *connection_uuid);
ByxConnectionPersistData *byx_config_manager_get_connection_persist_data (ByxConfigManager *self, const char *connection_uuid);

ByxServiceRunData *byx_config_manager_get_service_run_data (ByxConfigManager *self, const char *service_uuid);
ByxServicePersistData *byx_config_manager_get_service_persist_data (ByxConfigManager *self, const char *service_uuid);

void byx_config_manager_cleanup_persist_data(ByxConfigManager *self);

/*****************************************************************************/

/* Properties */
#define BYX_CONFIG_CMD_LINE_OPTIONS                  "cmd-line-options"

#define BYX_CONFIG_DEFAULT_CONNECTIVITY_INTERVAL 300
#define BYX_CONFIG_DEFAULT_CONNECTIVITY_RESPONSE "NetworkManager is online" /* NOT LOCALIZED */

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

#define BYX_CONFIG_KEYFILE_KEY_MAIN_DEBUG                    "debug"
#define BYX_CONFIG_KEYFILE_KEY_MAIN_SLAVES_ORDER             "slaves-order"
#define BYX_CONFIG_KEYFILE_KEY_LOGGING_BACKEND               "backend"
#define BYX_CONFIG_KEYFILE_KEY_CONFIG_ENABLE                 "enable"
#define BYX_CONFIG_KEYFILE_KEY_ATOMIC_SECTION_WAS            ".was"
#define BYX_CONFIG_KEYFILE_KEY_KEYFILE_PATH                  "path"
#define BYX_CONFIG_KEYFILE_KEY_KEYFILE_UNMANAGED_DEVICES     "unmanaged-devices"

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




/*****************************************************************************/

#endif /* __BYX_CONFIG_MANAGER_H__ */