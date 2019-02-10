/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

#ifndef __UTIL_STDIO_H__
#define __UTIL_STDIO_H__


#define util_sprintf_buf(buf, format, ...) \
	({ \
		int _buf_len; \
		\
		/* ensure that the buffer is statically allocated */ \
		assert (sizeof(buf) / sizeof((buf)[0]) == sizeof(buf) && sizeof(buf) != sizeof(char *)); \
		\
		_buf_len = snprintf (buf, sizeof(buf), ""format"", ##__VA_ARGS__); \
		if (_buf_len >= sizeof(buf)) { \
            NULL; \
        } else { \
            buf; \
        } \
	})

#if 0
#define nm_sprintf_bufa(n_elements, format, ...) \
	({ \
		char *_buf; \
		int _buf_len; \
		typeof (n_elements) _n_elements = (n_elements); \
		\
		_buf = g_alloca (_n_elements); \
		_buf_len = g_snprintf (_buf, _n_elements, \
		                       ""format"", ##__VA_ARGS__); \
		nm_assert (_buf_len < _n_elements); \
		_buf; \
	})
#endif

#endif /* __UTIL_STDIO_H__ */
