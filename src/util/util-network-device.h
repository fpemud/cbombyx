/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

#ifndef __UTIL_NETWORK_DEVICE_H__
#define __UTIL_NETWORK_DEVICE_H__

#include <glib.h>

gboolean util_network_device_set_up (int ifindex, GError **error);
void util_network_device_set_down (int ifindex);

#endif /* __UTIL_NETWORK_DEVICE_H__ */
