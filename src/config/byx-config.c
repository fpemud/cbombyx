/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

#include "byx-common.h"

#include "util/util-glib-keyfile.h"
#include "byx-config.h"

struct _ByxConfig {
	gboolean show_version;
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

static void _byx_cmd_line_options_parse(ByxConfig *config, int argc, char *argv[]);

ByxConfig *byx_config_new (int argc, char *argv[], GError **error)
{
	ByxConfig *config = g_try_new(ByxConfig, 1);
	if (config == NULL) {
		goto failure;
	}

	config->show_version = FALSE;
	config->log_level = NULL;
	config->log_domains = NULL;
	config->pidfile = BYX_PID_FILE;
	config->is_debug = FALSE;

	config->connectivity.enabled = FALSE;
	config->connectivity.uri = NULL;
	config->connectivity.response = NULL;
	config->connectivity.interval = 0;

	config->autoconnect_retries_default = 0;

	config->no_auto_default.arr = NULL;
	config->no_auto_default.specs = NULL;
	config->no_auto_default.specs_config = NULL;

	config->ignore_carrier = NULL;

	config->keyfile = util_keyfile_load_from_file (BYX_CONFIG_MAIN_FILE, error);
	if (config->keyfile == NULL) {
		goto failure;
	}

	_byx_cmd_line_options_parse(config, argc, argv);

	return config;

failure:
	byx_config_free (config);
	return NULL;
}

void byx_config_free (ByxConfig *config)
{
	if (config == NULL) {
		return;
	}

	g_free (config->log_level);
	g_free (config->log_domains);
	g_free (config->pidfile);

	g_free (&config->connectivity.uri);
	g_free (&config->connectivity.response);

	/* FIXME */
	config->no_auto_default.arr = NULL;
	config->no_auto_default.specs = NULL;
	config->no_auto_default.specs_config = NULL;
	config->ignore_carrier = NULL;
	config->keyfile = NULL;

	g_free(config);
}

/*****************************************************************************/

gboolean byx_config_get_show_version (ByxConfig *config)
{
    return config->show_version;
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

/*****************************************************************************/

static void _byx_cmd_line_options_parse(ByxConfig *config, int argc, char *argv[])
{
    GOptionEntry options[] = {
        {
            "debug",
			'd',
			0,
			G_OPTION_ARG_NONE,
            &config->is_debug,
			N_("Don't become a daemon, and log to stderr"),
            NULL,
        },
        {
			"connectivity-uri",
			0,
			0,
			G_OPTION_ARG_STRING,
			&config->connectivity.uri,
			N_("An http(s) address for checking internet connectivity"),
			BYX_CONNECTIVITY_URL,
		},
        {
			"connectivity-response",
			0,
			0,
			G_OPTION_ARG_STRING,
			&config->connectivity.response,
			N_("The expected start of the response"),
			BYX_CONNECTIVITY_RESPONSE
		},
        {
			"connectivity-interval",
			0,
			0,
			G_OPTION_ARG_INT,
			&config->connectivity.interval,
			N_("The interval between connectivity checks (in seconds)"),
			G_STRINGIFY (BYX_CONNECTIVITY_INTERVAL)
		},
		{
            "version",
            'V',
            0,
            G_OPTION_ARG_NONE,
            &config->show_version,
            N_("Print NetworkManager version and exit"),
            NULL,
        },
		{
            "log-level",
            0,
            0,
            G_OPTION_ARG_STRING,
            &config->log_level,
            N_("Log level: one of [TRACE,DEBUG,INFO,WARN,ERR,OFF,KEEP]"),       /* FIXME: should be provided by logging module */
            "INFO",
        },
		{
            "log-domains",
            0,
            0,
            G_OPTION_ARG_STRING,
            &config->log_domains,
            N_("Log domains separated by ',': any combination of [%s]"),
            "PLATFORM,RFKILL,WIFI",
        },
		{
            "pid-file",
			'p',
			0,
			G_OPTION_ARG_FILENAME,
            &config->pidfile,
            N_("Specify the location of a PID file"),
            BYX_PID_FILE,
        },
		{
            NULL
        },
	};

	char *summary = _("abc");
	/* _("NetworkManager monitors all network connections and automatically\nchooses the best connection to use.  It also allows the user to\nspecify wireless access points which wireless cards in the computer\nshould associate with."))) */

	GOptionContext *opt_ctx;
#if 0
	int i;
#endif

#if 0
	for (i = 0; options[i].long_name; i++) {
		NM_PRAGMA_WARNING_DISABLE("-Wformat-nonliteral")
		if (!strcmp (options[i].long_name, "log-level")) {
			opt_fmt_log_level = options[i].description;
			opt_loc_log_level = &options[i].description;
			options[i].description = g_strdup_printf (options[i].description, byx_logging_all_levels_to_string ());
		} else if (!strcmp (options[i].long_name, "log-domains")) {
			opt_fmt_log_domains = options[i].description;
			opt_loc_log_domains = &options[i].description;
			options[i].description = g_strdup_printf (options[i].description, byx_logging_all_domains_to_string ());
		}
		NM_PRAGMA_WARNING_REENABLE
	}
#endif

	/* Parse options */
	opt_ctx = g_option_context_new (NULL);
	g_option_context_set_translation_domain (opt_ctx, GETTEXT_PACKAGE);
	g_option_context_set_ignore_unknown_options (opt_ctx, FALSE);
	g_option_context_set_help_enabled (opt_ctx, TRUE);
	g_option_context_add_main_entries (opt_ctx, options, NULL);
	g_option_context_set_summary (opt_ctx, summary);

#if 0
	success = g_option_context_parse (opt_ctx, argc, argv, &error);
	if (!success) {
		fprintf (stderr, _("%s.  Please use --help to see a list of valid options.\n"),
		         error->message);
		g_clear_error (&error);
	}
	g_option_context_free (opt_ctx);

	if (opt_loc_log_level) {
		g_free ((char *) *opt_loc_log_level);
		*opt_loc_log_level = opt_fmt_log_level;
	}
	if (opt_loc_log_domains) {
		g_free ((char *) *opt_loc_log_domains);
		*opt_loc_log_domains = opt_fmt_log_domains;
	}
#endif
}
