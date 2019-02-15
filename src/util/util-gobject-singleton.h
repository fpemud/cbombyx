/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

#ifndef __UTIL_GOBJECT_SINGLETON_H__
#define __UTIL_GOBJECT_SINGLETON_H__

#define BYX_DEFINE_SINGLETON_GETTER(TYPE, GETTER, GTYPE, ...) \
    static TYPE *__singleton_instance__ = NULL; \
    \
    TYPE *GETTER (void) \
    { \
	    if (G_UNLIKELY (!__singleton_instance__)) { \
		    __singleton_instance__ = (g_object_new (GTYPE, ##__VA_ARGS__, NULL)); \
		    g_assert (__singleton_instance__ != NULL); \
		    /* nm_log_dbg (LOGD_CORE, "create %s singleton (%p)", G_STRINGIFY (TYPE), singleton_instance); */ \
    	} \
        return __singleton_instance__; \
    }

#endif /* __UTIL_GOBJECT_SINGLETON_H__ */
