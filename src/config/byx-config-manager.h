/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

#ifndef __BYX_CONFIG_MANAGER_H__
#define __BYX_CONFIG_MANAGER_H__

#include "byx-config.h"
#include "byx-run-data.h"
#include "byx-persist-data.h"
#include "byx-connection-run-data.h"
#include "byx-connection-persist-data.h"
#include "byx-service-run-data.h"
#include "byx-service-persist-data.h"

G_BEGIN_DECLS

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

/*****************************************************************************/

gboolean byx_config_manager_add_cmd_line_options (ByxConfigManager *self, int argc, char *argv[], GError **error);

void byx_config_manager_reload (ByxConfigManager *self);

ByxConfig *byx_config_manager_get_config(ByxConfigManager *self);

ByxRunData *byx_config_manager_get_run_data (ByxConfigManager *self);
ByxPersistData *byx_config_manager_get_persist_data (ByxConfigManager *self);

ByxConnectionRunData *byx_config_manager_get_connection_run_data (ByxConfigManager *self, const char *connection_uuid, GError **error);
ByxConnectionPersistData *byx_config_manager_get_connection_persist_data (ByxConfigManager *self, const char *connection_uuid, GError **error);

ByxServiceRunData *byx_config_manager_get_service_run_data (ByxConfigManager *self, const char *service_uuid);
ByxServicePersistData *byx_config_manager_get_service_persist_data (ByxConfigManager *self, const char *service_uuid);

void byx_config_manager_cleanup_persist_data(ByxConfigManager *self);

G_END_DECLS

#endif /* __BYX_CONFIG_MANAGER_H__ */
