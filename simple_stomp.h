#ifndef SIMPLE_STOMP_H
#define SIMPLE_STOMP_H

#define MAX_BUF 10000

typedef void (*error_handler)(char *msg);

struct simple_stomp_t
{
   int sd;
   error_handler eh;
   int ix;
   char buf[MAX_BUF];
};

typedef struct simple_stomp_t simple_stomp_t ;

void simple_stomp_debug(int debug);
int simple_stomp_init(simple_stomp_t *ctx, const char *host, int port, error_handler eh);
int simple_stomp_initx(simple_stomp_t *ctx, const char *host, int port,
                       const char *clientid, const char *un, const char *pw,
                       error_handler eh);
int simple_stomp_write(simple_stomp_t *ctx, const char *qname, const char *msg);
int simple_stomp_sub(simple_stomp_t *ctx, const char *qname);
int simple_stomp_unsub(simple_stomp_t *ctx, const char *qname);
int simple_stomp_readone(simple_stomp_t *ctx, char *msg);
int simple_stomp_read(simple_stomp_t *ctx, const char *qname, char *msg);
int simple_stomp_close(simple_stomp_t *ctx);

#endif
