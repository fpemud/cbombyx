/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

#include "byx-common.h"

#include <assert.h>

#include "util-glib-keyfile.h"

gboolean util_keyfile_get_boolean (GKeyFile *keyfile, const gchar *group_name, const gchar *key, gboolean default_value)
{
    gboolean ret = FALSE;
    g_autoptr(GError) local = NULL;

	ret = g_key_file_get_boolean (keyfile, group_name, key, &local);
    if (!ret && local)
        return default_value;

    return ret;
}
