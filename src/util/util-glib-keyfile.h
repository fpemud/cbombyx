/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

#ifndef __UTIL_GLIB_KEYFILE_H__
#define __UTIL_GLIB_KEYFILE_H__

gboolean util_keyfile_get_boolean (GKeyFile *keyfile, const gchar *group_name, const gchar *key, gboolean default_value);

#endif /* __UTIL_GLIB_KEYFILE_H__ */
