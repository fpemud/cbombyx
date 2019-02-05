/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

#ifndef __BYX_DEFAULT_VALUES_H__
#define __BYX_DEFAULT_VALUES_H__

#define BYX_BUILTIN_CONFIG_DIR                   BYX_DATADIR "/conf.d"
#define BYX_BUILTIN_CONNECTION_CONFIG_DIR        BYX_DATADIR "/connection.d"
#define BYX_BUILTIN_SERVICE_CONFIG_DIR           BYX_DATADIR "/service.d"

#define BYX_CONFIG_DIR                           BYX_CONFDIR
#define BYX_CONFIG_MAIN_FILE                     BYX_CONFDIR "/bombyx.conf"
#define BYX_CONNECTION_CONFIG_DIR                BYX_CONFDIR "/connection.d"
#define BYX_SERVICE_CONFIG_DIR                   BYX_CONFDIR "/service.d"

#define BYX_RUN_DATA_DIR                         BYX_RUNDIR
#define BYX_PID_FILE                             BYX_RUNDIR "/bombyx.pid"
#define BYX_RUN_DATA_FILE                        BYX_RUNDIR "/bombyx-intern.conf"
#define BYX_CONNECTION_RUN_DATA_DIR              BYX_RUNDIR "/connection.d"
#define BYX_SERVICE_RUN_DATA_DIR                 BYX_RUNDIR "/service.d"

#define BYX_PERSIST_DATA_DIR                     BYX_VARDIR
#define BYX_PERSIST_DATA_FILE                    BYX_VARDIR "/bombyx-intern.conf"
#define BYX_CONNECTION_PERSIST_DATA_DIR          BYX_VARDIR "/connection.d"
#define BYX_SERVICE_PERSIST_DATA_DIR             BYX_VARDIR "/service.d"

#define BYX_DEFAULT_CONNECTIVITY_INTERVAL        300
#define BYX_DEFAULT_CONNECTIVITY_RESPONSE        "NetworkManager is online" /* NOT LOCALIZED */

#endif /* __BYX_DEFAULT_VALUES_H__ */
