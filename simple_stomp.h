#ifndef SIMPLE_STOMP_H
#define SIMPLE_STOMP_H

#include <stddef.h>
#include <assert.h>

typedef struct simple_stomp simple_stomp_t ;
typedef void (*error_handler)(const simple_stomp_t *ctx);

#define MAX_BUF (4096*4 - sizeof(error_handler) - sizeof(size_t) - sizeof(int) - 256)

struct simple_stomp {
   error_handler err_handler;
   size_t recvsize;
   int sd;
   char recvbuf[MAX_BUF];
   char errmsg[256];
};
#if 0
static_assert(sizeof(simple_stomp_t) == 16384, "sizeof(simple_stomp_t)");
#endif

typedef  struct simple_string {
   const char *s;
   size_t len;
} simple_string_t;

typedef enum simple_stomp_result {
   SIMPLE_STOMP_TRUNCATED = -1,
   SIMPLE_STOMP_SUCCESS = 0,
   SIMPLE_STOMP_FAILURE = 1,
} simple_stomp_result_t;

void simple_stomp_debug(int debug);
simple_stomp_result_t simple_stomp_init(simple_stomp_t *ctx,
    const simple_string_t *host, int port,
    const simple_string_t *clientid,
    const simple_string_t *username,
    const simple_string_t *password,
    error_handler eh);
simple_stomp_result_t simple_stomp_write(simple_stomp_t *ctx, const simple_string_t *qname, const simple_string_t *msg);
simple_stomp_result_t simple_stomp_sub(simple_stomp_t *ctx, const simple_string_t *qname);
simple_stomp_result_t simple_stomp_unsub(simple_stomp_t *ctx, const simple_string_t *queueid);
simple_stomp_result_t simple_stomp_readone(simple_stomp_t *ctx, void *msg, size_t msgsize);
simple_stomp_result_t simple_stomp_read(simple_stomp_t *ctx, const simple_string_t *qname, void *msg, size_t msgsize);
simple_stomp_result_t simple_stomp_close(simple_stomp_t *ctx);

#endif
