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
 * Copyright (C) 2005 - 2011 Red Hat, Inc.
 * Copyright (C) 2006 - 2008 Novell, Inc.
 */

#ifndef __BYX_CONNECTION_MANAGER_H__
#define __BYX_CONNECTION_MANAGER_H__

#include "byx-connection.h"

#define BYX_TYPE_CONNECTION_MANAGER            (byx_connection_manager_get_type ())
#define BYX_CONNECTION_MANAGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BYX_TYPE_CONNECTION_MANAGER, ByxConnectionManager))
#define BYX_CONNECTION_MANAGER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BYX_TYPE_CONNECTION_MANAGER, ByxConnectionManagerClass))
#define BYX_IS_CONNECTION_MANAGER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BYX_TYPE_CONNECTION_MANAGER))
#define BYX_IS_CONNECTION_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BYX_TYPE_CONNECTION_MANAGER))
#define BYX_CONNECTION_MANAGER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BYX_TYPE_CONNECTION_MANAGER, ByxConnectionManagerClass))

typedef struct _ByxConnectionManager ByxConnectionManager;
typedef struct _ByxConnectionManagerClass ByxConnectionManagerClass;

GType byx_connection_manager_get_type (void);

ByxConnectionManager *byx_connection_manager_get (void);

CList *byx_connection_manager_get_activatable_connections (ByxConnectionManager *manager,
                                                           gboolean sort);

gboolean byx_connection_manager_activate_connection (ByxConnectionManager *manager,
                                                     ByxConnection *connection,
                                                     GError **error);

gboolean byx_connection_manager_deactivate_all_connections (ByxConnectionManager *manager,
                                                            GError **error);

#endif /* __BYX_CONNECTION_MANAGER_H__ */
