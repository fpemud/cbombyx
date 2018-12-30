/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

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
} ByxConfigCmdLineOptions;

/* for main.c only */
ByxConfigCmdLineOptions *byx_config_cmd_line_options_new (gboolean first_start);

void byx_config_cmd_line_options_free (ByxConfigCmdLineOptions *cli);

void byx_config_cmd_line_options_parse(ByxConfigCmdLineOptions *cli, int argc, char *argv[]);
