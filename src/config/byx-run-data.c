/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

#include "byx-default.h"

#include <assert.h>
#include "util/byx-glib-keyfile.h"
#include "byx-run-data.h"

/*****************************************************************************/

struct _ByxRunData {
    GKeyFile *keyfile;
    gchar *filename;
};
