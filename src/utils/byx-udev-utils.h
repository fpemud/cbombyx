/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* nm-udev-utils.h - udev utils functions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
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
 * Copyright (C) 2017 Red Hat, Inc.
 */

#ifndef __BYX_UDEV_UTILS_H__
#define __BYX_UDEV_UTILS_H__

struct udev;
struct udev_device;
struct udev_enumerate;

gboolean byx_udev_utils_property_as_boolean (const char *uproperty);
const char *byx_udev_utils_property_decode (const char *uproperty, char **to_free);
char       *byx_udev_utils_property_decode_cp (const char *uproperty);

typedef struct _NMPUdevClient ByxUdevClient;

typedef void (*ByxUdevClientEvent) (ByxUdevClient *udev_client,
                                   struct udev_device *udevice,
                                   gpointer event_user_data);

ByxUdevClient *byx_udev_client_new (const char *const*subsystems,
                                  ByxUdevClientEvent event_handler,
                                  gpointer event_user_data);

ByxUdevClient *byx_udev_client_unref (ByxUdevClient *self);

struct udev *byx_udev_client_get_udev (ByxUdevClient *self);

struct udev_enumerate *byx_udev_client_enumerate_new (ByxUdevClient *self);

#endif /* __BYX_UDEV_UTILS_H__ */
