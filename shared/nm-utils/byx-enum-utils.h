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
 * (C) Copyright 2017 Red Hat, Inc.
 */

#ifndef __BYX_ENUM_UTILS_H__
#define __BYX_ENUM_UTILS_H__

/*****************************************************************************/

typedef struct {
	/* currently, this is only used for byx_utils_enum_from_str_full() to
	 * declare additional aliases for values. */
	const char *nick;
	int value;
} ByxUtilsEnumValueInfo;

char *byx_utils_enum_to_str_full (GType type,
                                  int value,
                                  const char *sep,
                                  const ByxUtilsEnumValueInfo *value_infos);
gboolean byx_utils_enum_from_str_full (GType type,
                                       const char *str,
                                       int *out_value,
                                       char **err_token,
                                       const ByxUtilsEnumValueInfo *value_infos);

const char **byx_utils_enum_get_values (GType type, gint from, gint to);

/*****************************************************************************/

#endif /* __BYX_ENUM_UTILS_H__ */
