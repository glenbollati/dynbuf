#ifndef DYNBUF_H
#define DYNBUF_H

#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OK 1
#define ERR 0

struct dynbuf {
	char *data;
	size_t len, cap;
	// TODO size_t max_cap; // allocation limit
};

// TODO dynbuf *dynbuf_create(size_t initial_cap, size_t max_cap);
// TODO dynbuf *dynbuf_create_no_limit(size_t initial_cap);

struct dynbuf *dynbuf_create(size_t initial_cap);
void dynbuf_destroy(struct dynbuf *buf);
void dynbuf_clear(struct dynbuf *buf);
void dynbuf_soft_clear(struct dynbuf *buf);
void dynbuf_init(struct dynbuf *buf, size_t initial_cap);
void dynbuf_deinit(struct dynbuf *buf);
void dynbuf_reserve(struct dynbuf *buf, size_t size);
void dynbuf_resize(struct dynbuf *buf, size_t new_size);
void dynbuf_append(struct dynbuf *buf, const char *str, size_t len);
void dynbuf_append_char(struct dynbuf *buf, char c);
void dynbuf_terminate(struct dynbuf *buf);
void dynbuf_append_string(struct dynbuf *buf, const char *str);
void dynbuf_append_strings(struct dynbuf *buf, int count, ...);
int dynbuf_readfile(struct dynbuf *dst, const char *fpath);
int dynbuf_append_fmt(struct dynbuf *buf, const char *fmt, ...);
void dynbuf_tolower(struct dynbuf *buf);

inline static void *xmalloc(size_t size)
{
	void *x = malloc(size);
	if (!x) abort();
	return x;
}

inline static void *xcalloc(size_t count, size_t size)
{
	void *x = calloc(count, size);
	if (!x) abort();
	return x;
}

inline static void *xrealloc(void *x, size_t size)
{
	x = realloc(x, size);
	if (!x) abort();
	return x;
}

inline static bool dynbuf_is_string(struct dynbuf *buf)
{
	return (buf->data[buf->len - 1] == '\0');
}

struct dynbuf *dynbuf_create(size_t initial_cap)
{
	struct dynbuf *buf;
	buf = xmalloc(sizeof(*buf));
	dynbuf_init(buf, initial_cap);
	return buf;
}

void dynbuf_destroy(struct dynbuf *buf)
{
	assert(buf);
	dynbuf_deinit(buf);
	free(buf);
}

void dynbuf_clear(struct dynbuf *buf)
{
	assert(buf);
	memset(buf->data, 0, buf->len);
	buf->len = 0;
}

void dynbuf_soft_clear(struct dynbuf *buf)
{
	assert(buf);
	*buf->data = '\0';
	buf->len = 0;
}

void dynbuf_init(struct dynbuf *buf, size_t initial_cap)
{
	assert(buf);
	buf->len = 0;
	buf->cap = initial_cap;
	buf->data = xcalloc(initial_cap, sizeof(*buf->data));
}

void dynbuf_deinit(struct dynbuf *buf)
{
	assert(buf && buf->data);
	buf->len = buf->cap = 0;
	free(buf->data);
}

void dynbuf_reserve(struct dynbuf *buf, size_t size)
{
	if (buf->cap < size) dynbuf_resize(buf, size);
}

void dynbuf_resize(struct dynbuf *buf, size_t new_size)
{
	assert(buf && new_size);
	buf->cap = new_size;
	buf->data = xrealloc(buf->data, buf->cap);
}

void dynbuf_append(struct dynbuf *buf, const char *str, size_t len)
{
	if (!str || !len)
		return;

	if (buf->len + len > buf->cap) {
		buf->cap += len;
		buf->data = xrealloc(buf->data, buf->cap);
	}
	memcpy(buf->data + buf->len, str, len);
	buf->len += len;
}

void dynbuf_append_char(struct dynbuf *buf, char c)
{
	if (buf->len + 1 > buf->cap) {
		buf->cap += 1;
		buf->data = xrealloc(buf->data, buf->cap);
	}
	buf->data[buf->len++] = c;
}

void dynbuf_terminate(struct dynbuf *buf)
{
	if (buf) dynbuf_append_char(buf, '\0');
}

// NOTE: does NOT include NUL terminator
void dynbuf_append_string(struct dynbuf *buf, const char *str)
{
	if (str) dynbuf_append(buf, str, strlen(str));
}

void dynbuf_append_strings(struct dynbuf *buf, int count, ...)
{
	assert(buf);

	va_list va;
	va_start(va, count);

	const char *next = NULL;
	for (int i = 0; i < count; i++) {
		next = va_arg(va, const char *);
		dynbuf_append(buf, next, strlen(next));
	}

	va_end(va);
}

int dynbuf_readfile(struct dynbuf *dst, const char *fpath)
{
	FILE *f = fopen(fpath, "r");
	if (!f) goto err;

	// if dynamic buffer is not initialised, do it now
	// if (!dst->data)
	// 	dynbuf_init(dst, 1024);

	char buf[1024];
	size_t nread = 0;

	while ((nread = fread(buf, sizeof(*buf), sizeof(buf), f)) && !ferror(f))
		dynbuf_append(dst, buf, nread);

	if (ferror(f)) goto err;
	// dst->data[dst->len] = '\0';

	fclose(f);
	return OK;

err:
	if (f) fclose(f);
	return ERR;
}

int dynbuf_append_fmt(struct dynbuf *buf, const char *fmt, ...)
{
	assert(buf);

	char *tmp = NULL;
	va_list va;

	va_start(va, fmt);
	int n = vsnprintf(tmp, 0, fmt, va);
	va_end(va);

	if (n < 0) return ERR;

	size_t size = (size_t)n + 1; // +1 for NUL terminator

	if (size > (buf->cap - buf->len)) {
		buf->cap += (size - (buf->cap - buf->len));
		buf->data = xrealloc(buf->data, buf->cap * sizeof(*buf->data));
	}

	va_start(va, fmt);
	n = vsnprintf(buf->data + buf->len, (buf->cap - buf->len), fmt, va);
	va_end(va);

	if (n < 0) return ERR;

	// NOTE: we leave NUL terminator for safety but do not include it in the length.
	buf->len += (size_t)n;

	return OK;
}

void dynbuf_tolower(struct dynbuf *buf)
{
	for (size_t i = 0; i < buf->len; i++)
		buf->data[i] = tolower(buf->data[i]);
}

#endif // DYNBUF_H
