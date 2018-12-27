/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */



struct ByxConfigCmdLineOptions {
	char *config_main_file;
	char *intern_config_file;
	char *config_dir;
	char *system_config_dir;
	char *state_file;
	gboolean is_debug;
	char *connectivity_uri;

	/* We store interval as signed internally to track whether it's
	 * set or not via GOptionEntry
	 */
	int connectivity_interval;
	char *connectivity_response;

	/* @first_start is not provided by command line. It is a convenient hack
	 * to pass in an argument to ByxConfigManager. This makes ByxConfigCmdLineOptions a
	 * misnomer.
	 *
	 * It is true, if NM is started the first time -- contrary to a restart
	 * during the same boot up. That is determined by the content of the
	 * /var/run/NetworManager state directory. */
	bool first_start;
};
