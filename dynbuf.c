#include "dynbuf.h"

int main(void)
{
	struct dynbuf *buf = dynbuf_create(1024);
	dynbuf_append_strings(buf, 2, "hello", "world");
	dynbuf_destroy(buf);

	return 0;
}
