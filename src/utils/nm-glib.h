/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/*
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
 * Copyright 2008 - 2018 Red Hat, Inc.
 */

#ifndef __NM_GLIB_H__
#define __NM_GLIB_H__

#include <gio/gio.h>
#include <string.h>

#include "gsystem-local-alloc.h"

#ifdef __clang__

#undef G_GNUC_BEGIN_IGNORE_DEPRECATIONS
#undef G_GNUC_END_IGNORE_DEPRECATIONS

#define G_GNUC_BEGIN_IGNORE_DEPRECATIONS \
    _Pragma("clang diagnostic push") \
    _Pragma("clang diagnostic ignored \"-Wdeprecated-declarations\"")

#define G_GNUC_END_IGNORE_DEPRECATIONS \
    _Pragma("clang diagnostic pop")

#endif

#if !GLIB_CHECK_VERSION(2, 44, 0)
static inline gpointer
g_steal_pointer (gpointer pp)
{
	gpointer *ptr = (gpointer *) pp;
	gpointer ref;

	ref = *ptr;
	*ptr = NULL;

	return ref;
}

/* type safety */
#define g_steal_pointer(pp) \
  (0 ? (*(pp)) : (g_steal_pointer) (pp))
#endif

static inline gboolean
_nm_g_strv_contains (const gchar * const *strv,
                     const gchar         *str)
{
#if !GLIB_CHECK_VERSION(2, 44, 0)
	g_return_val_if_fail (strv != NULL, FALSE);
	g_return_val_if_fail (str != NULL, FALSE);

	for (; *strv != NULL; strv++) {
		if (g_str_equal (str, *strv))
			return TRUE;
	}

	return FALSE;
#else
	G_GNUC_BEGIN_IGNORE_DEPRECATIONS
	return g_strv_contains (strv, str);
	G_GNUC_END_IGNORE_DEPRECATIONS
#endif
}
#define g_strv_contains _nm_g_strv_contains

#if !GLIB_CHECK_VERSION (2, 56, 0)
#define g_object_ref(Obj)      ((typeof(Obj)) g_object_ref (Obj))
#define g_object_ref_sink(Obj) ((typeof(Obj)) g_object_ref_sink (Obj))
#endif

#ifndef g_autofree
/* we still don't rely on recent glib to provide g_autofree. Hence, we continue
 * to use our gs_* free macros that we took from libgsystem.
 *
 * To ease migration towards g_auto*, add a compat define for g_autofree. */
#define g_autofree gs_free
#endif

#endif  /* __NM_GLIB_H__ */
