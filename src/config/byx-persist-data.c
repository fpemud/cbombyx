/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

#include "byx-default.h"

#include <assert.h>

#include "util/byx-glib-keyfile.h"
#include "byx-persist-data.h"

/*****************************************************************************/

struct _ByxPersistData {
    GKeyFile *keyfile;
    gchar *filename;
};

/*****************************************************************************/

#define assert_persist_data_valid(data) \
	assert((data) != NULL && (data)->keyfile != NULL && (data)->filename != NULL)

static const gchar *network_type_to_key_string(ByxNetworkType network_type)
{
	switch(network_type) {
		case BYX_NETWORK_TYPE_WIRED:
			return "NetworkTypeWiredEnabled";
		case BYX_NETWORK_TYPE_WIRELESS:
			return "NetworkTypeWirelessEnabled";
		case BYX_NETWORK_TYPE_MOBILE:
			return "NetworkTypeMobileEnabled";
		default:
			assert(FALSE);
	}
}

/*****************************************************************************/

ByxPersistData *byx_persist_data_new()
{
	ByxPersistData *ret = NULL;

	ret = (ByxPersistData*)malloc(sizeof(*ret));
	return ret;
}

void byx_persist_data_free(ByxPersistData *data)
{
	assert_persist_data_valid(data);

    g_key_file_unref(data->keyfile);
	free(data->filename);
	free(data);
}

gboolean byx_persist_data_save(ByxPersistData *data, GError **error)
{
	gboolean success = FALSE;
	GError *local = NULL;

	assert_persist_data_valid(data);

	success = g_key_file_save_to_file(data->keyfile, data->filename, &local);
	if (!success)
		g_propagate_error(error, local);
	return success;
}

gboolean byx_persist_data_get_global_switch(ByxPersistData *data)
{
	assert_persist_data_valid(data);

	return byx_keyfile_get_boolean (data->keyfile, "main", "NetworkEnabled", TRUE);
}

void byx_persist_data_set_global_switch(ByxPersistData *data, gboolean value)
{
	assert_persist_data_valid(data);

	g_key_file_set_boolean(data->keyfile, "main", "NetworkEnabled", value);
}

gboolean byx_persist_data_get_network_type_switch(ByxPersistData *data, ByxNetworkType network_type)
{
	assert_persist_data_valid(data);

	return byx_keyfile_get_boolean (data->keyfile, "main", network_type_to_key_string(network_type), TRUE);
}

void byx_persist_data_set_network_type_switch(ByxPersistData *data, ByxNetworkType network_type, gboolean value)
{
	assert_persist_data_valid(data);

	g_key_file_set_boolean(data->keyfile, "main", network_type_to_key_string(network_type), value);
}
