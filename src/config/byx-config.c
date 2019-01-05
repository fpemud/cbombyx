/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

static void _cmd_line_options_clear (ByxConfig *config)
{
	config->is_debug = FALSE;
	g_clear_pointer (&config->connectivity_uri, g_free);
	g_clear_pointer (&config->connectivity_response, g_free);
	config->connectivity_interval = -1;
	config->first_start = FALSE;
}

void byx_cmd_line_options_parse(ByxCmdLineOptions *config, int argc, char **argv[])
{
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

	/* Parse options */
	opt_ctx = g_option_context_new (NULL);
	g_option_context_set_translation_domain (opt_ctx, GETTEXT_PACKAGE);
	g_option_context_set_ignore_unknown_options (opt_ctx, FALSE);
	g_option_context_set_help_enabled (opt_ctx, TRUE);
	g_option_context_add_main_entries (opt_ctx, options, NULL);
	g_option_context_set_summary (opt_ctx, summary);

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
}



static void _byx_cmd_line_options_add_to_entries (ByxConfig *config, GOptionContext *opt_ctx) {

    GOptionEntry config_options[] = {
        {
            "debug", 'd', 0, G_OPTION_ARG_NONE,
            &config->is_debug, N_("Don't become a daemon, and log to stderr"),
            NULL,
        },

        /* These three are hidden for now, and should eventually just go away. */
        { "connectivity-uri", 0, G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_STRING, &config->connectivity_uri, N_("An http(s) address for checking internet connectivity"), "http://example.com" },
        { "connectivity-interval", 0, G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_INT, &config->connectivity_interval, N_("The interval between connectivity checks (in seconds)"), G_STRINGIFY (BYX_CONFIG_DEFAULT_CONNECTIVITY_INTERVAL) },
        { "connectivity-response", 0, G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_STRING, &config->connectivity_response, N_("The expected start of the response"), BYX_CONFIG_DEFAULT_CONNECTIVITY_RESPONSE },
        { 0 },
    };

	GOptionEntry options2[] = {
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
            "no-daemon",
            'n',
            G_OPTION_FLAG_REVERSE,
            G_OPTION_ARG_NONE,
            &config->become_daemon,
            N_("Don't become a daemon"),
            NULL,
        },
		{
            "log-level",
            0,
            0,
            G_OPTION_ARG_STRING,
            &config->opt_log_level,
            N_("Log level: one of [TRACE,DEBUG,INFO,WARN,ERR,OFF,KEEP]"),       /* FIXME: should be provided by logging module */
            "INFO",
        },
		{
            "log-domains",
            0,
            0,
            G_OPTION_ARG_STRING,
            &config->opt_log_domains,
            N_("Log domains separated by ',': any combination of [%s]"),
            "PLATFORM,RFKILL,WIFI"
        },
		{
            "pid-file", 'p', 0, G_OPTION_ARG_FILENAME,
            &config->pidfile,
            N_("Specify the location of a PID file"),
            BYX_PID_FILE,
        },
		{
            NULL
        },
	};

    g_option_context_add_main_entries (opt_ctx, config_options, NULL);
    g_option_context_add_main_entries (opt_ctx, config_options2, NULL);
}






const char *byx_config_get_log_level (ByxConfig *config)
{
    assert (config != NULL);

    return config->log_level;
}

const char *byx_config_get_log_domains (ByxConfig *config)
{
    assert (config != NULL);

    return config->log_domains;
}

gboolean byx_config_get_is_debug (ByxConfig *config)
{
    assert (config != NULL);

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
    gs_free gchar *debug = NULL;
    guint flags = 0;
    const char *env = getenv ("NM_DEBUG");
    GError *local = NULL;

    debug = g_key_file_get_string(config->keyfile, "main", "debug", &local);

    flags |= _parse_debug_string (env, keys, G_N_ELEMENTS (keys));
    flags |= _parse_debug_string (debug, keys, G_N_ELEMENTS (keys));

    return flags;
}

/*****************************************************************************/

gboolean byx_config_get_is_first_start (ByxConfig *config)
{
    assert (config != NULL);

    return config->first_start;
}