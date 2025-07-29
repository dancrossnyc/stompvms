#ifndef VMS_SIMPLE_STOMP_H
#define VMS_SIMPLE_STOMP_H

#include <descrip.h>

#include "simple_stomp.h"

void vms_simple_stomp_debug(int *debug);
int vms_simple_stomp_init(simple_stomp_t *ctx, struct dsc$descriptor *host, int *port);
int vms_simple_stomp_write(simple_stomp_t *ctx, struct dsc$descriptor *qname, struct dsc$descriptor *msg);
int vms_simple_stomp_sub(simple_stomp_t *ctx, struct dsc$descriptor *qname);
int vms_simple_stomp_unsub(simple_stomp_t *ctx, struct dsc$descriptor *qname);
int vms_simple_stomp_readone(simple_stomp_t *ctx, struct dsc$descriptor *msg, int *msglen);
int vms_simple_stomp_read(simple_stomp_t *ctx, struct dsc$descriptor *qname, struct dsc$descriptor *msg, int *msglen);
int vms_simple_stomp_close(simple_stomp_t *ctx);

#endif
