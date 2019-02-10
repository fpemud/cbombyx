/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

#ifndef __BYX_CONNECTION_PERSIST_DATA__
#define __BYX_CONNECTION_PERSIST_DATA__

G_BEGIN_DECLS

typedef struct _ByxConnectionPersistData ByxConnectionPersistData;

ByxConnectionPersistData *byx_connection_persist_data_new(const char *connnection_uuid, GError **error);
void byx_connection_persist_data_free(ByxConnectionPersistData *data);

GKeyFile *byx_connection_persist_data_get_keyfile(ByxConnectionPersistData *data);

G_END_DECLS

#endif /* __BYX_CONNECTION_PERSIST_DATA__ */
