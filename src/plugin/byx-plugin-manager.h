/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

#ifndef __BYX_PLUGIN_MANAGER_H__
#define __BYX_PLUGIN_MANAGER_H__

G_BEGIN_DECLS

#define BYX_TYPE_PLUGIN_MANAGER            (byx_plugin_manager_get_type ())
#define BYX_PLUGIN_MANAGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BYX_TYPE_PLUGIN_MANAGER, ByxPluginManager))
#define BYX_PLUGIN_MANAGER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BYX_TYPE_PLUGIN_MANAGER, ByxPluginManagerClass))
#define BYX_IS_PLUGIN_MANAGER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BYX_TYPE_PLUGIN_MANAGER))
#define BYX_IS_PLUGIN_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BYX_TYPE_PLUGIN_MANAGER))
#define BYX_PLUGIN_MANAGER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BYX_TYPE_PLUGIN_MANAGER, ByxPluginManagerClass))

typedef struct _ByxPluginManager ByxPluginManager;
typedef struct _ByxPluginManagerClass ByxPluginManagerClass;

GType byx_plugin_manager_get_type(void);

ByxPluginManager *byx_plugin_manager_get(void);

G_END_DECLS

#endif /* __BYX_PLUGIN_MANAGER_H__ */
