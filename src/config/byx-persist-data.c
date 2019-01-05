/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

ByxPersistData *byx_persist_data_new()
{
	ByxPersistData *ret = NULL;

	ret = (ByxPersistData*)malloc(sizeof(*ret));
	return ret;
}

void byx_persist_data_free(ByxPersistData *data)
{
	assert(data != NULL && data->keyfile != NULL);

	free(data->keyfile);
	free(data);
}

gboolean byx_persist_data_save(ByxPersistData *data)
{
	gboolean success = FALSE;
	GError *local = NULL;

	assert(self != NULL);
	assert(data != NULL && data->keyfile != NULL && data->filename != NULL);

	success = g_key_file_save_to_file(data->keyfile, data->filename, &local);
	_LOGD("write config data file \"%s\"%s%s", data->filename, success ? "" : ": ", success ? "" : local->message);
	if (!success)
		g_propagate_error(error, local);
	return success;
}

