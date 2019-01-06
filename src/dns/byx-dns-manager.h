/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

#ifndef __BYX_DNS_MANAGER_H__
#define __BYX_DNS_MANAGER_H__

#define BYX_TYPE_DNS_MANAGER            (byx_dns_manager_get_type ())
#define BYX_DNS_MANAGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BYX_TYPE_DNS_MANAGER, ByxDnsManager))
#define BYX_DNS_MANAGER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), BYX_TYPE_DNS_MANAGER, ByxDnsManagerClass))
#define BYX_IS_DNS_MANAGER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BYX_TYPE_DNS_MANAGER))
#define BYX_IS_DNS_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BYX_TYPE_DNS_MANAGER))
#define BYX_DNS_MANAGER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BYX_TYPE_DNS_MANAGER, ByxDnsManagerClass))

typedef struct _ByxDnsManager ByxDnsManager;
typedef struct _ByxDnsManagerClass ByxDnsManagerClass;

GType byx_dns_manager_get_type (void);

ByxDnsManager *byx_dns_manager_get (void);

gboolean byx_dns_manager_start(ByxDnsManager *self, GError **error);

void byx_dns_manager_stop (ByxDnsManager *self);

#endif /* __BYX_DNS_MANAGER_H__ */
