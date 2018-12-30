/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

static void _byx_config_cmd_line_options_clear (ByxConfigCmdLineOptions *cli)
{
	cli->is_debug = FALSE;
	g_clear_pointer (&cli->connectivity_uri, g_free);
	g_clear_pointer (&cli->connectivity_response, g_free);
	cli->connectivity_interval = -1;
	cli->first_start = FALSE;
}

ByxConfigCmdLineOptions *byx_config_cmd_line_options_new (gboolean first_start)
{
	ByxConfigCmdLineOptions *cli = g_new0 (ByxConfigCmdLineOptions, 1);

	_byx_config_cmd_line_options_clear (cli);

	cli->first_start = first_start;

	return cli;
}

void byx_config_cmd_line_options_free (ByxConfigCmdLineOptions *cli)
{
    assert (cli != NULL);

	_byx_config_cmd_line_options_clear (cli);
	g_free (cli);
}

void byx_config_cmd_line_options_parse(ByxConfigCmdLineOptions *cli, int argc, char **argv[])
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



void byx_config_cmd_line_options_add_to_entries (ByxConfigCmdLineOptions *cli, GOptionContext *opt_ctx) {

    GOptionEntry config_options[] = {
        {
            "debug", 'd', 0, G_OPTION_ARG_NONE,
            &cli->is_debug, N_("Don't become a daemon, and log to stderr"),
            NULL,
        },

        /* These three are hidden for now, and should eventually just go away. */
        { "connectivity-uri", 0, G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_STRING, &cli->connectivity_uri, N_("An http(s) address for checking internet connectivity"), "http://example.com" },
        { "connectivity-interval", 0, G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_INT, &cli->connectivity_interval, N_("The interval between connectivity checks (in seconds)"), G_STRINGIFY (BYX_CONFIG_DEFAULT_CONNECTIVITY_INTERVAL) },
        { "connectivity-response", 0, G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_STRING, &cli->connectivity_response, N_("The expected start of the response"), BYX_CONFIG_DEFAULT_CONNECTIVITY_RESPONSE },
        { 0 },
    };

	GOptionEntry options2[] = {
		{
            "version",
            'V',
            0,
            G_OPTION_ARG_NONE,
            &cli->show_version,
            N_("Print NetworkManager version and exit"),
            NULL,
        },
		{
            "no-daemon",
            'n',
            G_OPTION_FLAG_REVERSE,
            G_OPTION_ARG_NONE,
            &cli->become_daemon,
            N_("Don't become a daemon"),
            NULL,
        },
		{
            "log-level",
            0,
            0,
            G_OPTION_ARG_STRING,
            &cli->opt_log_level,
            N_("Log level: one of [TRACE,DEBUG,INFO,WARN,ERR,OFF,KEEP]"),       /* FIXME: should be provided by logging module */
            "INFO",
        },
		{
            "log-domains",
            0,
            0,
            G_OPTION_ARG_STRING,
            &cli->opt_log_domains,
            N_("Log domains separated by ',': any combination of [%s]"),
            "PLATFORM,RFKILL,WIFI"
        },
		{
            "pid-file", 'p', 0, G_OPTION_ARG_FILENAME,
            &cli->pidfile,
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


