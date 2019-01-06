/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

#ifndef __BYX_CONFIG__
#define __BYX_CONFIG__

#define BYX_TYPE_CONFIG            (byx_config_get_type ())
#define BYX_CONFIG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BYX_TYPE_CONFIG, ByxConfig))
#define BYX_CONFIG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  BYX_TYPE_CONFIG, ByxConfigClass))
#define BYX_IS_CONFIG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BYX_TYPE_CONFIG))
#define BYX_IS_CONFIG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  BYX_TYPE_CONFIG))
#define BYX_CONFIG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  BYX_TYPE_CONFIG, ByxConfigClass))

typedef struct _ByxConfig ByxConfig;
typedef struct _ByxConfigClass ByxConfigClass;

GType byx_config_get_type (void);

ByxConfig *byx_config_new (gboolean first_start);

void byx_config_free (ByxConfig *self);

gboolean byx_config_get_show_version (ByxConfig *self);

gboolean byx_config_get_become_daemon (ByxConfig *self);

const char *byx_config_get_log_level (ByxConfig *self);

const char *byx_config_get_log_domains (ByxConfig *self);

const char *byx_config_get_pidfile (ByxConfig *self);

gboolean byx_config_get_is_debug (ByxConfig *self);

gboolean byx_config_get_is_first_start (ByxConfig *self);

/*****************************************************************************/

enum {
	BYX_CONFIG_DEBUG_FLAG_RLIMIT_CORE =    (1 << 0),
	BYX_CONFIG_DEBUG_FLAG_FATAL_WARNINGS = (1 << 1),
} ByxConfigDebugFlags;

guint byx_config_get_debug_flags(ByxConfig *self);



#endif /* __BYX_CONFIG__ */
