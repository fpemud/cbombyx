/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

#include "byx-common.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "byx-config-manager.h"

#define KEYFILE_LIST_SEPARATOR ','

struct _ByxConfigManagerClass {
    GObjectClass parent;
};

typedef struct {
    ByxConfig *config;

    ByxRunData *run_data;
    ByxPersistData *persist_data;

    GHashTable *connection_run_data;
    GHashTable *connection_persist_data;

    GHashTable *service_run_data;
    GHashTable *service_persist_data;

#if 1
    char **atomic_section_prefixes;
#endif

} ByxConfigManagerPrivate;

struct _ByxConfigManager {
    GObject parent;
    ByxConfigManagerPrivate _priv;
};

G_DEFINE_TYPE_WITH_PRIVATE (ByxConfigManager, byx_config_manager, G_TYPE_OBJECT)

/*****************************************************************************/

static ByxConfigManager *_singleton_instance = NULL;

ByxConfigManager *byx_config_manager_setup (int argc, char *argv[], GError **error)
{
    ByxConfigManagerPrivate *priv = NULL;

    assert (_singleton_instance == NULL);

    _singleton_instance = g_object_new (BYX_TYPE_CONFIG_MANAGER, NULL);
    assert (_singleton_instance != NULL);

    priv = byx_config_manager_get_instance_private (_singleton_instance);

    priv->config = byx_config_new(argc, argv);
    assert (priv->config != NULL);                              /* FIXME */

    priv->run_data = byx_run_data_new(!g_file_test (BYX_RUNDIR, G_FILE_TEST_IS_DIR));
    assert (priv->run_data != NULL);                            /* FIXME */

    priv->persist_data = byx_persist_data_new();
    assert (priv->persist_data != NULL);                        /* FIXME */

    return _singleton_instance;
}

ByxConfigManager *byx_config_manager_get (void)
{
    assert (_singleton_instance != NULL);

    return _singleton_instance;
}

/*****************************************************************************/

static void byx_config_manager_dispose (GObject *gobject);

static void byx_config_manager_class_init (ByxConfigManagerClass *config_manager_class)
{
    GObjectClass *object_class = G_OBJECT_CLASS (config_manager_class);

    object_class->dispose = byx_config_manager_dispose;
}

static void byx_config_manager_init (ByxConfigManager *self)
{
}

static void byx_config_manager_dispose (GObject *gobject)
{
    ByxConfigManagerPrivate *priv = byx_config_manager_get_instance_private ((ByxConfigManager *) gobject);

    g_clear_object (&priv->persist_data);
    g_clear_object (&priv->run_data);
    g_clear_object (&priv->config);

    G_OBJECT_CLASS (byx_config_manager_parent_class)->dispose (gobject);
}

/*****************************************************************************/

ByxConfig *byx_config_manager_get_config(ByxConfigManager *self)
{
    ByxConfigManagerPrivate *priv = byx_config_manager_get_instance_private(self);
    return priv->config;
}

/*****************************************************************************/

ByxRunData *byx_config_manager_get_run_data (ByxConfigManager *self)
{
    ByxConfigManagerPrivate *priv = byx_config_manager_get_instance_private(self);
    return priv->run_data;
}

ByxPersistData *byx_config_manager_get_persist_data (ByxConfigManager *self)
{
    ByxConfigManagerPrivate *priv = byx_config_manager_get_instance_private(self);
    return priv->persist_data;
}

/*****************************************************************************/

ByxConnectionRunData *byx_config_manager_get_connection_run_data (ByxConfigManager *self, const char *connection_uuid, GError **error)
{
    ByxConfigManagerPrivate *priv = byx_config_manager_get_instance_private(self);
    char *new_uuid = NULL;
    ByxConnectionRunData *data = NULL;
    GError *local = NULL;

    assert (connection_uuid != NULL);

    data = g_hash_table_lookup(priv->connection_run_data, connection_uuid);
    if (data != NULL) {
        return data;
    }

    new_uuid = strdup(connection_uuid);
    if (new_uuid == NULL) {
        goto failure;
    }

    data = byx_connection_run_data_new(connection_uuid, &local);
    if (data == NULL) {
        goto failure;
    }

    g_hash_table_insert (priv->connection_run_data, new_uuid, data);

    return data;

failure:
    free (new_uuid);
    byx_connection_run_data_free (data);
    g_propagate_error (error, local);
    return NULL;
}

ByxConnectionPersistData *byx_config_manager_get_connection_persist_data (ByxConfigManager *self, const char *connection_uuid, GError **error)
{
    ByxConfigManagerPrivate *priv = byx_config_manager_get_instance_private(self);
    char *new_uuid = NULL;
    ByxConnectionPersistData *data = NULL;
    GError *local = NULL;

    assert (connection_uuid != NULL);

    data = g_hash_table_lookup(priv->connection_persist_data, connection_uuid);
    if (data != NULL) {
        return data;
    }

    new_uuid = strdup(connection_uuid);
    if (new_uuid == NULL) {
        goto failure;
    }

    data = byx_connection_persist_data_new(connection_uuid, &local);
    if (data == NULL) {
        goto failure;
    }

    g_hash_table_insert (priv->connection_persist_data, new_uuid, data);

    return data;

failure:
    free (new_uuid);
    byx_connection_persist_data_free (data);
    g_propagate_error (error, local);
    return NULL;
}

/*****************************************************************************/

void byx_config_manager_cleanup_persist_data(ByxConfigManager *self)
{
    /* iterate all the file in BYX_CONNECTION_RUN_DATA_DIR and BYX_CONNECTION_PERSIST_DATA_DIR */
    /* to see delete data that has no corresponding connection */
}
