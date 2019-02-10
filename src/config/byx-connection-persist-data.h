/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

#ifndef __BYX_CONNECTION_PERSIST_DATA__
#define __BYX_CONNECTION_PERSIST_DATA__

typedef struct {
    GKeyFile *keyfile;
    gchar *filename;
} ByxConnectionPersistData;

ByxConnectionPersistData *byx_connection_persist_data_new(void);
void byx_connection_persist_data_free(ByxConnectionPersistData *data);

#endif /* __BYX_CONNECTION_PERSIST_DATA__ */
