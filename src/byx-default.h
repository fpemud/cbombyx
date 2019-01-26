/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* NetworkManager -- Network link manager
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 * (C) Copyright 2015 Red Hat, Inc.
 */

#ifndef __BYX_DEFAULT_H__
#define __BYX_DEFAULT_H__

/*****************************************************************************/

#ifdef G_LOG_DOMAIN
#error Do not define G_LOG_DOMAIN
#endif
#define G_LOG_DOMAIN "bombyx"

/*****************************************************************************/

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n.h>
#include <gio/gio.h>

#ifndef BYX_MORE_ASSERTS
#define BYX_MORE_ASSERTS 0
#endif

#if BYX_MORE_ASSERTS == 0
/* The cast macros like NM_TYPE() are implemented via G_TYPE_CHECK_INSTANCE_CAST()
 * and _G_TYPE_CIC(). The latter, by default performs runtime checks of the type
 * by calling g_type_check_instance_cast().
 * This check has a certain overhead without being helpful.
 *
 * Example 1:
 *     static void foo (NMType *obj)
 *     {
 *         access_obj_without_check (obj);
 *     }
 *     foo ((NMType *) obj);
 *     // There is no runtime check and passing an invalid pointer
 *     // leads to a crash.
 *
 * Example 2:
 *     static void foo (NMType *obj)
 *     {
 *         access_obj_without_check (obj);
 *     }
 *     foo (NM_TYPE (obj));
 *     // There is a runtime check which prints a g_warning(), but that doesn't
 *     // avoid the crash as NM_TYPE() cannot do anything then passing on the
 *     // invalid pointer.
 *
 * Example 3:
 *     static void foo (NMType *obj)
 *     {
 *         g_return_if_fail (NM_IS_TYPE (obj));
 *         access_obj_without_check (obj);
 *     }
 *     foo ((NMType *) obj);
 *     // There is a runtime check which prints a g_critical() which also avoids
 *     // the crash. That is actually helpful to catch bugs and avoid crashes.
 *
 * Example 4:
 *     static void foo (NMType *obj)
 *     {
 *         g_return_if_fail (NM_IS_TYPE (obj));
 *         access_obj_without_check (obj);
 *     }
 *     foo (NM_TYPE (obj));
 *     // The runtime check is performed twice, with printing a g_warning() and
 *     // a g_critical() and avoiding the crash.
 *
 * Example 3 is how it should be done. Type checks in NM_TYPE() are pointless.
 * Disable them for our production builds.
 */
#ifndef G_DISABLE_CAST_CHECKS
#define G_DISABLE_CAST_CHECKS
#endif
#endif

#if BYX_MORE_ASSERTS == 0
#ifndef G_DISABLE_CAST_CHECKS
/* Unless compiling with G_DISABLE_CAST_CHECKS, glib performs type checking
 * during G_VARIANT_TYPE() via g_variant_type_checked_(). This is not necesary
 * because commonly this cast is needed during something like
 *
 *   g_variant_builder_init (&props, G_VARIANT_TYPE ("a{sv}"));
 *
 * Note that in if the variant type would be invalid, the check still
 * wouldn't make the buggy code magically work. Instead of passing a
 * bogus type string (bad), it would pass %NULL to g_variant_builder_init()
 * (also bad).
 *
 * Also, a function like g_variant_builder_init() already validates
 * the input type via something like
 *
 *   g_return_if_fail (g_variant_type_is_container (type));
 *
 * So, by having G_VARIANT_TYPE() also validate the type, we validate
 * twice, whereas the first validation is rather pointless because it
 * doesn't prevent the function to be called with invalid arguments.
 *
 * Just patch G_VARIANT_TYPE() to perform no check.
 */
#undef G_VARIANT_TYPE
#define G_VARIANT_TYPE(type_string) ((const GVariantType *) (type_string))
#endif
#endif

/*****************************************************************************/

#if BYX_MORE_ASSERTS == 0

/* glib assertions (g_return_*(), g_assert*()) contain a textual representation
 * of the checked statement. This part of the assertion blows up the size of the
 * binary. Unless we compile a debug-build with BYX_MORE_ASSERTS, drop these
 * parts. Note that the failed assertion still prints the file and line where the
 * assertion fails. That shall suffice. */

static inline void
_nm_g_return_if_fail_warning (const char *log_domain,
                              const char *file,
                              int line)
{
	char file_buf[256 + 15];

	g_snprintf (file_buf, sizeof (file_buf), "((%s:%d))", file, line);
	g_return_if_fail_warning (log_domain, file_buf, "<dropped>");
}

#define g_return_if_fail_warning(log_domain, pretty_function, expression) \
	_nm_g_return_if_fail_warning (log_domain, __FILE__, __LINE__)

#undef g_return_val_if_reached
#define g_return_val_if_reached(val) \
    G_STMT_START { \
        g_log (G_LOG_DOMAIN, \
               G_LOG_LEVEL_CRITICAL, \
               "file %s: line %d (%s): should not be reached", \
               __FILE__, \
               __LINE__, \
               "<dropped>"); \
        return (val); \
    } G_STMT_END

#undef g_return_if_reached
#define g_return_if_reached() \
    G_STMT_START { \
        g_log (G_LOG_DOMAIN, \
               G_LOG_LEVEL_CRITICAL, \
               "file %s: line %d (%s): should not be reached", \
               __FILE__, \
               __LINE__, \
               "<dropped>"); \
        return; \
    } G_STMT_END

#endif

/*****************************************************************************/

#include "common/byx-internal-macros.h"
#include "common/byx-version-macros.h"
#include "common/byx-types.h"
#include "common/byx-logging.h"

/*****************************************************************************/

#endif /* __BYX_DEFAULT_H__ */
