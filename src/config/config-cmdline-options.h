/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */


#define DEFAULT_SYSTEM_CONFIG_DIR            NMLIBDIR "/conf.d"
#define DEFAULT_SYSTEM_CONNECTION_CONFIG_DIR NMLIBDIR "/connection.d"
#define DEFAULT_SYSTEM_SERVICE_CONFIG_DIR    NMLIBDIR "/service.d"

#define DEFAULT_CONFIG_MAIN_FILE             NMCONFDIR "/bombyx.conf"
#define DEFAULT_CONFIG_DIR                   NMCONFDIR "/conf.d"
#define DEFAULT_CONNECTION_CONFIG_DIR        NMCONFDIR "/connection.d"
#define DEFAULT_SERVICE_CONFIG_DIR           NMCONFDIR "/service.d"

#define RUN_DATA_FILE                        NMRUNDIR "/bombyx-intern.conf"
#define CONNECTION_RUN_DATA_DIR              NMRUNDIR "/connection.d"
#define SERVICE_RUN_DATA_DIR                 NMRUNDIR "/service.d"

#define PERSIST_DATA_FILE                    NMSTATEDIR "/bombyx-intern.conf"
#define CONNECTION_PERSIST_DATA_DIR          NMSTATEDIR "/connection.d"
#define SERVICE_PERSIST_DATA_DIR             NMSTATEDIR "/service.d"

typedef struct {
	char *system_config_dir;
	char *system_connection_config_dir;
	char *system_service_config_dir;

	char *config_main_file;
	char *config_dir;
	char *connection_config_dir;
	char *service_config_dir;

	char *run_data_file;
	char *connection_run_data_dir;
	char *service_run_data_dir;

	char *persist_data_file;
	char *connection_persist_data_dir;
	char *service_persist_data_dir;

	gboolean show_version;
	gboolean print_config;
	gboolean become_daemon;
	gboolean g_fatal_warnings;
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
