/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

#ifndef __BYX_RUN_DATA__
#define __BYX_RUN_DATA__

typedef struct _ByxRunData ByxRunData;

/*****************************************************************************/

ByxRunData *byx_run_data_new (gboolean first_start);

void byx_run_data_free (ByxRunData *data);

gboolean byx_run_data_get_is_first_start (ByxRunData *data);

#endif /* __BYX_RUN_DATA__ */
