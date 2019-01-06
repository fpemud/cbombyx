/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* NetworkManager -- Network link manager
 *
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
 * Copyright (C) 2014 Red Hat, Inc.
 */

#ifndef __MAIN_UTILS_H__
#define __MAIN_UTILS_H__

void     byx_main_utils_ensure_root (void);

void     byx_main_utils_setup_signals (GMainLoop *main_loop);

void     byx_main_utils_ensure_statedir (void);

void     byx_main_utils_ensure_rundir (void);

void     byx_main_utils_ensure_no_running_pidfile (const char *pidfile);

gboolean byx_main_utils_write_pidfile (const char *pidfile, GError **error);

#endif /* __MAIN_UTILS_H__ */
