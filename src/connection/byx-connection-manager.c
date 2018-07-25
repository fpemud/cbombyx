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
 * Copyright (C) 2005 - 2012 Red Hat, Inc.
 * Copyright (C) 2006 - 2008 Novell, Inc.
 */

#include "nm-default.h"

#include "byx-connection-manager.h"

#include <string.h>

#include "byx-connection.h"

struct _ByxConnectionManagerClass {
	GObjectClass parent;
};

typedef struct {
	PeasEngine *plugin_engine;
	GSList *connection_list;
} ByxConnectionManagerPrivate;

struct _ByxConnectionManager {
	GObject parent;
	ByxConnectionManagerPrivate _priv;
};

G_DEFINE_TYPE (ByxConnectionManager, byx_connection_manager, G_TYPE_OBJECT)

#define BYX_CONNECTION_MANAGER_GET_PRIVATE(self) _BYX_GET_PRIVATE (self, ByxConnectionManager, BYX_IS_CONNECTION_MANAGER)

/*****************************************************************************/

BYX_DEFINE_SINGLETON_GETTER (ByxConnectionManager, byx_connection_manager_get, BYX_TYPE_CONNECTION_MANAGER);

static void
byx_connection_manager_class_init (ByxConnectionManagerClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = byx_connection_manager_dispose;
}

static void
byx_connection_manager_init (ByxConnectionManager *self)
{
	ByxConnectionManagerPrivate *priv = BYX_CONNECTION_MANAGER_GET_PRIVATE (self);

	priv->plugin_engine = peas_engine_new();
	peas_engine_enable_loader(priv->plugin_engine, "python3");
	peas_engine_add_search_path(priv->plugin_engine, "");			/* fixme */

	priv->connection_list = NULL									/* fixme */




}

static void
byx_connection_manager_dispose (GObject *object)
{
	ByxConnectionManager *self = (ByxConnectionManager *) object;
	ByxConnectionManagerPrivate *priv = BYX_CONNECTION_MANAGER_GET_PRIVATE (self);

	g_object_unref (priv->connection_list);
	g_object_unref (priv->plugin_engine);

	G_OBJECT_CLASS (byx_connection_manager_parent_class)->dispose (object);
}
