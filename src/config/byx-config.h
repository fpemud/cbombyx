/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

#ifndef __BYX_CONFIG__
#define __BYX_CONFIG__

G_BEGIN_DECLS

typedef struct _ByxConfig ByxConfig;

ByxConfig *byx_config_new (int argc, char *argv[], GError **error);

void byx_config_free (ByxConfig *config);

gboolean byx_config_get_show_version (ByxConfig *config);

const char *byx_config_get_log_level (ByxConfig *config);

const char *byx_config_get_log_domains (ByxConfig *config);

const char *byx_config_get_pidfile (ByxConfig *config);

gboolean byx_config_get_is_debug (ByxConfig *config);

/*****************************************************************************/

enum {
	BYX_CONFIG_DEBUG_FLAG_RLIMIT_CORE =    (1 << 0),
	BYX_CONFIG_DEBUG_FLAG_FATAL_WARNINGS = (1 << 1),
} ByxConfigDebugFlags;

guint byx_config_get_debug_flags(ByxConfig *config);

G_END_DECLS

#endif /* __BYX_CONFIG__ */
