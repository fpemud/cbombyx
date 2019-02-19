/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

#include "byx-common.h"

#include <assert.h>

#include "util/util-stdio.h"
#include "util/util-glib-keyfile.h"
#include "byx-connection-persist-data.h"

#define KEYFILE_LIST_SEPARATOR ','

struct _ByxConnectionPersistData {
    GKeyFile *keyfile;
    gchar *filename;
};

ByxConnectionPersistData *byx_connection_persist_data_new (const char *connection_uuid, GError **error)
{
    ByxConnectionPersistData *data = NULL;
    GError *local = NULL;

    assert (connection_uuid != NULL);

    data = g_try_new0(ByxConnectionPersistData, 1);
    if (data == NULL) {
        goto failure;
    }

    data->filename = g_try_malloc(1024);
    if (data->filename == NULL) {
        goto failure;
    }

    data->keyfile = g_key_file_new ();
    if (data->keyfile == NULL) {
        goto failure;
    }

    if (util_snprintf (data->filename, 1024, "%s/%s", BYX_CONNECTION_PERSIST_DATA_DIR, connection_uuid) == NULL) {
        _LOGT ("failed to sprintf");
        goto failure;
    }

    g_key_file_set_list_separator (data->keyfile, KEYFILE_LIST_SEPARATOR);
    if (access (data->filename, F_OK) == 0) {
        if (!g_key_file_load_from_file (data->keyfile, data->filename, G_KEY_FILE_KEEP_COMMENTS, &local)) {
            _LOGT ("failed reading connection persist data \"%s\"", data->filename);
            goto failure;
        }
    }

    return data;

failure:
    byx_connection_persist_data_free (data);
    g_propagate_error (error, local);
    return NULL;
}

void byx_connection_persist_data_free (ByxConnectionPersistData *data)
{
    if (data != NULL) {
        util_key_file_free (data->keyfile);
        g_free (data->filename);
    }
    g_free (data);
}

GKeyFile *byx_connection_persist_data_get_keyfile(ByxConnectionPersistData *data)
{
    assert (data != NULL);
    return data->keyfile;
}
