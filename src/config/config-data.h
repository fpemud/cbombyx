/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

#ifndef __BYX_CONFIG_DATA_H__
#define __BYX_CONFIG_DATA_H__

typedef struct {
    GKeyFile *keyfile;
    gchar filename[MAXPATHLEN];
} ByxConfigData;

/*****************************************************************************/

gboolean byx_config_data_get_global_switch (ByxConfigData *data);
void byx_config_data_set_global_switch (ByxConfigData *self);

/*****************************************************************************/

typedef enum {
	BYX_CONFIG_DATA_NETWORK_TYPE_WIRED,
	BYX_CONFIG_DATA_NETWORK_TYPE_WIRELESS,
	BYX_CONFIG_DATA_NETWORK_TYPE_MOBILE,
} ByxConfigDataNetworkType;

gboolean byx_config_data_get_network_type_switch (ByxConfigData *data, ByxConfigDataNetworkType network_type);
void byx_config_data_set_network_type_switch (ByxConfigData *self, ByxConfigDataNetworkType network_type);

/*****************************************************************************/

gboolean byx_config_data_get_connectivity_check_state (ByxConfigData *data);
void byx_config_data_set_connectivity_check_state (ByxConfigData *data, gboolean enabled);

/*****************************************************************************/

void byx_config_data_save(ByxConfigData *data);
void byx_config_data_free(ByxConfigData *data);


#endif /* __BYX_CONFIG_DATA_H__ */
