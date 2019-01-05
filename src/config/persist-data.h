/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

#ifndef __BYX_PERSIST_DATA__
#define __BYX_PERSIST_DATA__

typedef struct {
    GKeyFile *keyfile;
    gchar filename[MAXPATHLEN];
} ByxPersistData;

/*****************************************************************************/

ByxPersistData *byx_persist_data_new();
void byx_persist_data_free(ByxPersistData *data);

void byx_persist_data_save(ByxPersistData *data);

/*****************************************************************************/

gboolean byx_persist_data_get_global_switch (ByxPersistData *data);
void byx_persist_data_set_global_switch (ByxPersistData *self);

/*****************************************************************************/

typedef enum {
	BYX_CONFIG_DATA_NETWORK_TYPE_WIRED,
	BYX_CONFIG_DATA_NETWORK_TYPE_WIRELESS,
	BYX_CONFIG_DATA_NETWORK_TYPE_MOBILE,
} ByxConfigDataNetworkType;

gboolean byx_persist_data_get_network_type_switch (ByxPersistData *data, ByxConfigDataNetworkType network_type);
void byx_persist_data_set_network_type_switch (ByxPersistData *self, ByxConfigDataNetworkType network_type);

/*****************************************************************************/

gboolean byx_persist_data_get_connectivity_check_state (ByxPersistData *data);
void byx_persist_data_set_connectivity_check_state (ByxPersistData *data, gboolean enabled);

/*****************************************************************************/

#endif /* __BYX_PERSIST_DATA__ */
