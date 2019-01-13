/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

#ifndef __BYX_CONNECTION_PLUGIN_H__
#define __BYX_CONNECTION_PLUGIN_H__

#include <netinet/in.h>

#define BYX_TYPE_CONNECTION_PLUGIN            (byx_connection_get_type ())
#define BYX_CONNECTION_PLUGIN(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BYX_TYPE_CONNECTION_PLUGIN, ByxConnectionPlugin))
#define BYX_CONNECTION_PLUGIN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  BYX_TYPE_CONNECTION_PLUGIN, ByxConnectionPluginClass))
#define BYX_IS_CONNECTION_PLUGIN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BYX_TYPE_CONNECTION_PLUGIN))
#define BYX_IS_CONNECTION_PLUGIN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  BYX_TYPE_CONNECTION_PLUGIN))
#define BYX_CONNECTION_PLUGIN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  BYX_TYPE_CONNECTION_PLUGIN, ByxConnectionPluginClass))

typedef struct _ByxConnectionPlugin ByxConnectionPlugin;
typedef struct _ByxConnectionPluginClass ByxConnectionPluginClass;

GType byx_connection_plugin_get_type (void);




#endif /* __BYX_CONNECTION_PLUGIN_H__ */
