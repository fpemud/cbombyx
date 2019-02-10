/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

#ifndef __BYX_CONNECTION_RUN_DATA__
#define __BYX_CONNECTION_RUN_DATA__

typedef struct {
    GKeyFile *keyfile;
    gchar *filename;
} ByxConnectionRunData;

ByxConnectionRunData *byx_connection_run_data_new(void);
void byx_connection_run_data_free(ByxConnectionRunData *data);

#endif /* __BYX_CONNECTION_RUN_DATA__ */
