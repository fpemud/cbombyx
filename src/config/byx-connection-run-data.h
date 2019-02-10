/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

#ifndef __BYX_CONNECTION_RUN_DATA__
#define __BYX_CONNECTION_RUN_DATA__

typedef struct _ByxConnectionRunData ByxConnectionRunData;

ByxConnectionRunData *byx_connection_run_data_new(const char *connnection_uuid, GError **error);
void byx_connection_run_data_free(ByxConnectionRunData *data);

GKeyFile *byx_connection_run_data_get_keyfile(ByxConnectionRunData *data);

#endif /* __BYX_CONNECTION_RUN_DATA__ */
