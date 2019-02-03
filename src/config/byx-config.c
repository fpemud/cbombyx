/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

#include "byx-default.h"

#include "byx-config.h"

struct _ByxConfig {
	char *self_description;

	gboolean show_version;
	gboolean become_daemon;
	char *log_level;
	char *log_domains;
	char *pidfile;

	gboolean is_debug;

	struct {
		gboolean enabled;
		char *uri;
		char *response;
		guint interval;
	} connectivity;

	int autoconnect_retries_default;

	struct {
		char **arr;
		GSList *specs;
		GSList *specs_config;
	} no_auto_default;

	GSList *ignore_carrier;

    GKeyFile *keyfile;
};

/*****************************************************************************/

ByxConfig *byx_config_new (void)
{
	return NULL;
}

void byx_config_free (ByxConfig *config)
{
}

/*****************************************************************************/

gboolean byx_config_get_show_version (ByxConfig *config)
{
    return config->show_version;
}

gboolean byx_config_get_become_daemon (ByxConfig *config)
{
    return config->become_daemon;
}

const char *byx_config_get_log_level (ByxConfig *config)
{
    return config->log_level;
}

const char *byx_config_get_log_domains (ByxConfig *config)
{
    return config->log_domains;
}

const char *byx_config_get_pidfile (ByxConfig *config)
{
    return config->pidfile;
}

gboolean byx_config_get_is_debug (ByxConfig *config)
{
    return config->is_debug;
}

/*****************************************************************************/

static gboolean _debug_key_matches (const gchar *key, const gchar *token, guint length)
{
	/* may not call GLib functions: see note in g_parse_debug_string() */
	for (; length; length--, key++, token++) {
		char k = (*key   == '_') ? '-' : g_ascii_tolower (*key  );
		char t = (*token == '_') ? '-' : g_ascii_tolower (*token);

		if (k != t)
			return FALSE;
	}

	return *key == '\0';
}

/* Similar to g_parse_debug_string(), but does not special case "help" or "all" */
static guint _parse_debug_string (const char *string, const GDebugKey *keys, guint nkeys)
{
	guint i;
	guint result = 0;
	const char *q;

	if (string == NULL)
		return 0;

	while (*string) {
		q = strpbrk (string, ":;, \t");
		if (!q)
			q = string + strlen (string);

		for (i = 0; i < nkeys; i++) {
			if (_debug_key_matches (keys[i].key, string, q - string))
				result |= keys[i].value;
		}

		string = q;
		if (*string)
			string++;
	}

	return result;
}

guint byx_config_get_debug_flags(ByxConfig *config)
{
    GDebugKey keys[] = {
        { "RLIMIT_CORE", BYX_CONFIG_DEBUG_FLAG_RLIMIT_CORE },
        { "fatal-warnings", BYX_CONFIG_DEBUG_FLAG_FATAL_WARNINGS },
    };
    g_autofree gchar *debug = NULL;
    guint flags = 0;
    const char *env = getenv ("NM_DEBUG");
    GError *local = NULL;

    debug = g_key_file_get_string(config->keyfile, "main", "debug", &local);

    flags |= _parse_debug_string (env, keys, G_N_ELEMENTS (keys));
    flags |= _parse_debug_string (debug, keys, G_N_ELEMENTS (keys));

    return flags;
}
