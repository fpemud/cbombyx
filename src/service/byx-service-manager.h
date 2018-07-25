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

#ifndef __BYX_SERVICE_MANAGER_H__
#define __BYX_SERVICE_MANAGER_H__

#include "byx-service.h"

#define BYX_TYPE_SERVICE_MANAGER            (byx_service_manager_get_type ())
#define BYX_SERVICE_MANAGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BYX_TYPE_SERVICE_MANAGER, ByxServiceManager))
#define BYX_SERVICE_MANAGER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BYX_TYPE_SERVICE_MANAGER, ByxServiceManagerClass))
#define BYX_IS_SERVICE_MANAGER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BYX_TYPE_SERVICE_MANAGER))
#define BYX_IS_SERVICE_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BYX_TYPE_SERVICE_MANAGER))
#define BYX_SERVICE_MANAGER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BYX_TYPE_SERVICE_MANAGER, ByxServiceManagerClass))

typedef struct _ByxServiceManager ByxServiceManager;
typedef struct _ByxServiceManagerClass ByxServiceManagerClass;

GType byx_service_manager_get_type (void);

ByxServiceManager *byx_service_manager_get (void);

gboolean nm_service_manager_activate_service (ByxServiceManager *manager,
                                              ByxService *vpn,
                                              GError **error);

#endif /* __BYX_SERVICE_MANAGER_H__ */
