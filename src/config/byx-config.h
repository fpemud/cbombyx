/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

#ifndef __BYX_CONFIG__
#define __BYX_CONFIG__

typedef struct {
	gboolean show_version;
	gboolean become_daemon;
	char *opt_log_level;
	char *opt_log_domains;
	char *pidfile;

	gboolean is_debug;
	char *connectivity_uri;

	/* We store interval as signed internally to track whether it's
	 * set or not via GOptionEntry
	 */
	int connectivity_interval;
	char *connectivity_response;

	gboolean first_start;

    GKeyFile *keyfile;
} ByxConfig;

ByxConfig *byx_config_new (gboolean first_start);

void byx_config_free (ByxConfig *config);

const char *byx_config_get_log_level (ByxConfig *config);

const char *byx_config_get_log_domains (ByxConfig *config);

gboolean byx_config_get_is_debug (ByxConfig *config);

/*****************************************************************************/

enum {
	BYX_CONFIG_DEBUG_FLAG_RLIMIT_CORE =    (1 << 0),
	BYX_CONFIG_DEBUG_FLAG_FATAL_WARNINGS = (1 << 1),
} ByxConfigDebugFlags;

guint byx_config_get_debug_flags(ByxConfig *config);

/*****************************************************************************/

gboolean byx_config_get_is_first_start (ByxConfig *config);



#endif /* __BYX_CONFIG__ */
