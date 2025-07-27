#include <assert.h>

#define XDEBUG
#include "simple_stomp.c"

void
errprint(const simple_stomp_t *ctx)
{
	printf("%s\n", ctx->errmsg);
}

int
main(void)
{
	const simple_string_t qname = MAKE_STRING("aqueue");
	const Header destination = { .name = MAKE_STRING("destination"), .data = &qname };
	const simple_string_t msg = MAKE_STRING("This is a test message.");
	Frame frame;
	makeframe(&frame, MAKE_STRING("CONNECT"), &destination , 1, &msg);
	makeframe(&frame, MAKE_STRING("CONNECT"), NULL, 0, &msg);
	makeframe(&frame, MAKE_STRING("CONNECT"), NULL, 0, NULL);
	simple_stomp_t ctx;
	memset(&ctx, 0, sizeof(ctx));
	memcpy(ctx.recvbuf, "C\n\nBODY", 8);
	ctx.recvsize = 8;
	ctx.err_handler = errprint;
	assert(recv_frame(&ctx, &frame) == SIMPLE_STOMP_SUCCESS);
	assert(strcmp(frame.cmd, "C") == 0);
	assert(strcmp(frame.headers, "") == 0);
	assert(strcmp(frame.body, "BODY") == 0);
#define FRAME	"MESSAGE\nH: D\n\nTHIS IS THE MESSAGE\0"
	memcpy(ctx.recvbuf, FRAME, sizeof(FRAME));
	ctx.recvsize = sizeof(FRAME);
	assert(recv_frame(&ctx, &frame) == SIMPLE_STOMP_SUCCESS);
	assert(strcmp(frame.cmd, "MESSAGE") == 0);
	assert(strcmp(frame.headers, "H: D\n") == 0);
	assert(strcmp(frame.body, "THIS IS THE MESSAGE") == 0);
	memcpy(ctx.recvbuf, "CONNECTED\n\n", 12);
	ctx.recvsize = 12;
	assert(recv_frame(&ctx, &frame) == SIMPLE_STOMP_SUCCESS);
	assert(strcmp(frame.cmd, "CONNECTED") == 0);
	assert(strcmp(frame.headers, "") == 0);
	assert(strcmp(frame.body, "") == 0);
	return 0;
}
