/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

#ifndef __UTIL_GLIB_KEYFILE_H__
#define __UTIL_GLIB_KEYFILE_H__

#include <glib.h>

GKeyFile *util_keyfile_load_from_file (const char *filename, GError **error);
GKeyFile *util_keyfile_load_from_directory_and_file (const char *directory, const char *filename, GError **error);

gboolean util_keyfile_get_boolean (GKeyFile *key_file, const gchar *group_name, const gchar *key, gboolean default_value);

void util_keyfile_free (GKeyFile *key_file);

#endif /* __UTIL_GLIB_KEYFILE_H__ */
