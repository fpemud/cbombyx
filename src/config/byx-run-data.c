/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

#include "byx-default.h"

#include <assert.h>
#include "util/byx-glib-keyfile.h"
#include "byx-run-data.h"

/*****************************************************************************/

struct _ByxRunData {
    GKeyFile *keyfile;
    gchar *filename;

    /*
     * It is true, if Bombyx is started the first time -- contrary to a restart
     * during the same boot up. That is determined by the content of the
     * /var/run/bombyx state directory. */
    gboolean first_start;
};

/*****************************************************************************/

#define _assert_run_data_valid(data) \
	assert((data) != NULL && (data)->keyfile != NULL && (data)->filename != NULL)

/*****************************************************************************/

ByxRunData *byx_run_data_new(gboolean first_start)
{
	ByxRunData *ret = NULL;

	ret = (ByxRunData*)malloc(sizeof(*ret));

    ret->first_start = first_start;

	return ret;
}

void byx_run_data_free(ByxRunData *data)
{
	_assert_run_data_valid(data);

    g_key_file_unref(data->keyfile);
	free(data->filename);
	free(data);
}

gboolean byx_run_data_get_is_first_start (ByxRunData *data)
{
	_assert_run_data_valid(data);

    return data->first_start;
}
