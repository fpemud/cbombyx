/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

#include "byx-common.h"

#include <assert.h>
#include "util-stdio.h"
#include "util-glib-keyfile.h"

#define KEYFILE_LIST_SEPARATOR ','

GKeyFile *util_keyfile_load_from_file (const char *filename, GError **error)
{
    GKeyFile *keyfile = NULL;

    keyfile = g_key_file_new ();
    if (keyfile == NULL) {
        return NULL;
    }

    g_key_file_set_list_separator (keyfile, KEYFILE_LIST_SEPARATOR);
    if (access (filename, F_OK) == 0) {
        if (!g_key_file_load_from_file (keyfile, filename, G_KEY_FILE_KEEP_COMMENTS, error)) {
            util_keyfile_free (keyfile);
            return NULL;
        }
    }

    return keyfile;
}

GKeyFile *util_keyfile_load_from_directory_and_file (const char *directory, const char *filename, GError **error)
{
    char full_filename[1024];
    GKeyFile *keyfile = NULL;
    GError *local = NULL;

    if (util_snprintf (full_filename, 1024, "%s/%s", directory, filename) == NULL) {
        return NULL;
    }

    keyfile = util_keyfile_load_from_file (full_filename, error);
    if (keyfile == NULL) {
        return NULL;
    }
    
    return keyfile;
}

gboolean util_keyfile_get_boolean (GKeyFile *key_file, const gchar *group_name, const gchar *key, gboolean default_value)
{
    gboolean ret = FALSE;
    g_autoptr(GError) local = NULL;

	ret = g_key_file_get_boolean (key_file, group_name, key, &local);
    if (!ret && local)
        return default_value;

    return ret;
}

void util_keyfile_free (GKeyFile *key_file)
{
    if (key_file == NULL)
        return;

    /* assert reference count == 1 */
    g_key_file_free(key_file);
}
