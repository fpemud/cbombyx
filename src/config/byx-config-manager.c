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
	char *builtin_config_dir;
	char *builtin_connection_config_dir;
	char *builtin_service_config_dir;

	char *config_dir;
	char *config_main_file;
	char *connection_config_dir;
	char *service_config_dir;

	char *run_data_file;
	char *connection_run_data_dir;
	char *service_run_data_dir;

	char *persist_data_file;
	char *connection_persist_data_dir;
	char *service_persist_data_dir;

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

#define BYX_CONFIG_GET_PRIVATE(self) _BYX_GET_PRIVATE (self, ByxConfigManager, BYX_IS_CONFIG_MANAGER)

/*****************************************************************************/

static ByxConfigManager *_singleton_instance = NULL;

ByxConfigManager *byx_config_manager_setup (int argc, char *argv[], GError **error)
{
    ByxConfigManagerPrivate *priv = NULL;

    assert (_singleton_instance == NULL);

    _singleton_instance = g_object_new (BYX_TYPE_CONFIG_MANAGER, NULL);
    assert (_singleton_instance != NULL);

    priv = byx_config_manager_get_instance_private (_singleton_instance);

    priv->builtin_config_dir = BYX_BUILTIN_CONFIG_DIR;
	priv->builtin_connection_config_dir = BYX_BUILTIN_CONNECTION_CONFIG_DIR;
	priv->builtin_service_config_dir = BYX_BUILTIN_SERVICE_CONFIG_DIR;

	priv->config_main_file = BYX_CONFIG_MAIN_FILE;
	priv->config_dir = BYX_CONFIG_DIR;
	priv->connection_config_dir = BYX_CONNECTION_CONFIG_DIR;
	priv->service_config_dir = BYX_SERVICE_CONFIG_DIR;

	priv->run_data_file = BYX_RUN_DATA_FILE;
	priv->connection_run_data_dir = BYX_CONNECTION_RUN_DATA_DIR;
	priv->service_run_data_dir = BYX_SERVICE_RUN_DATA_DIR;

	priv->persist_data_file = BYX_PERSIST_DATA_FILE;
	priv->connection_persist_data_dir = BYX_CONNECTION_PERSIST_DATA_DIR;
	priv->service_persist_data_dir = BYX_SERVICE_PERSIST_DATA_DIR;

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

static void byx_config_manager_finalize (GObject *gobject);

static void byx_config_manager_class_init (ByxConfigManagerClass *config_manager_class)
{
    GObjectClass *object_class = G_OBJECT_CLASS (config_manager_class);

    object_class->finalize = byx_config_manager_finalize;
}

static void byx_config_manager_init (ByxConfigManager *self)
{
}

static void byx_config_manager_finalize (GObject *gobject)
{
    ByxConfigManagerPrivate *priv = byx_config_manager_get_instance_private ((ByxConfigManager *) gobject);

    g_clear_object (&priv->persist_data);
    g_clear_object (&priv->run_data);
    g_clear_object (&priv->config);

    G_OBJECT_CLASS (byx_config_parent_class)->finalize (gobject);
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

    assert (uuid != NULL);

    data = g_hash_table_lookup(priv->connection_run_data, uuid);
    if (data != NULL) {
        return data;
    }

    new_uuid = strdup(uuid);
    if (new_uuid == NULL) {
        _LOGT ("failed to allocate memory");
        goto failure;
    }

    data = byx_connection_run_data_new();
    if (data == NULL) {
        _LOGT ("failed to allocate memory");
        goto failure;
    }

    if (util_sprintf_buf (data->filename, "%s/%s", priv->connection_run_data_dir, uuid) == NULL) {
        _LOGT ("failed to sprintf");
        goto failure;
    }

    data->keyfile = g_key_file_new ();                            /* FIXME: return NULL? */
    g_key_file_set_list_separator (data->keyfile, KEYFILE_LIST_SEPARATOR);

    if (access (path, F_OK) == 0) {
        if (!g_key_file_load_from_file (data->keyfile, data->filename, G_KEY_FILE_KEEP_COMMENTS, &local)) {
            _LOGT ("failed reading connection run data \"%s\"", data->filename);
            goto failure;
        }
    }

    g_hash_table_insert (priv->connection_run_data, new_uuid, data);

    return data;

failure:
    if (new_uuid != NULL)
        free (new_uuid);
    if (data != NULL)
        byx_connection_run_data_free (data);
    if (local != NULL)
        g_propagate_error (&error, local);
    return NULL;
}

ByxConnectionPersistData *byx_config_manager_get_connection_persist_data (ByxConfigManager *self, const char *connection_uuid)
{
    ByxConfigManagerPrivate *priv = byx_config_manager_get_instance_private(self);
    char *new_uuid = NULL;
    ByxConnectionPersistData *data = NULL;
    GError *local = NULL;

    assert (uuid != NULL);

    data = g_hash_table_lookup(priv->connection_persist_data, uuid);
    if (data != NULL) {
        return data;
    }

    new_uuid = strdup(uuid);
    if (new_uuid == NULL) {
        _LOGT ("failed to allocate memory");
        goto failure;
    }

    data = byx_connection_persist_data_new();
    if (data == NULL) {
        _LOGT ("failed to allocate memory");
        goto failure;
    }

    if (util_sprintf_buf (data->filename, "%s/%s", priv->connection_persist_data_dir, uuid) == NULL) {
        _LOGT ("failed to sprintf");
        goto failure;
    }

    data->keyfile = g_key_file_new ();                            /* FIXME: return NULL? */
    g_key_file_set_list_separator (data->keyfile, KEYFILE_LIST_SEPARATOR);

    if (access (path, F_OK) == 0) {
        if (!g_key_file_load_from_file (data->keyfile, data->filename, G_KEY_FILE_KEEP_COMMENTS, &local)) {
            _LOGT ("failed reading connection persist data \"%s\"", data->filename);
            goto failure;
        }
    }

    g_hash_table_insert (priv->connection_persist_data, new_uuid, data);

    return data;

failure:
    if (new_uuid != NULL)
        free (new_uuid);
    if (data != NULL)
        byx_connection_persist_data_free (data);
    if (local != NULL)
        g_propagate_error (&error, local);
    return NULL;
}

/*****************************************************************************/

void byx_config_manager_cleanup_persist_data(ByxConfigManager *self)
{
    /* iterate all the file in BYX_CONNECTION_RUN_DATA_DIR and BYX_CONNECTION_PERSIST_DATA_DIR */
    /* to see delete data that has no corresponding connection */
}
