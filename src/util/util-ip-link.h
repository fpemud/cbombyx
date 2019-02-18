/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

#ifndef __UTIL_IP_LINK_H__
#define __UTIL_IP_LINK_H__

gboolean util_ip_link_set_up (int ifindex, GError **error);
void util_ip_link_set_down (int ifindex);

#endif /* __UTIL_IP_LINK_H__ */
