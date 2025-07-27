#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "vms_simple_stomp.h"

static void
print(const simple_stomp_t *ctx)
{
    printf("%s\n", ctx->errmsg);
}

static simple_string_t
desc2string(struct dsc$descriptor *desc)
{
    short tmp;
    size_t len;
    const char *s;

    s = desc->dsc$a_pointer;
    switch (desc->dsc$b_class) {
    case DSC$K_CLASS_S:
    case DSC$K_CLASS_D:
        len = desc->dsc$w_length;
        break;
    case DSC$K_CLASS_VS:
        memcpy(&tmp, s, sizeof(short));
        len = tmp;
        s += sizeof(short);
        break;
    default:
        fprintf(stderr, "Unsupported descriptor class: %d\n", desc->dsc$b_class);
        exit(44);
    }

    return (simple_string_t){ .s = s, .len = len };
}

void vms_simple_stomp_debug(int *debug)
{
    simple_stomp_debug(*debug);
}


int vms_simple_stomp_init(simple_stomp_t *ctx,
    struct dsc$descriptor *host_dsc, int *pport,
    struct dsc$descriptor *client_id_dsc)
{
    simple_string_t host = desc2string(host_dsc);
    simple_string_t clientid = desc2string(client_id_dsc);
    return simple_stomp_init(ctx, &host, *pport, &clientid, print);
}

int vms_simple_stomp_write(simple_stomp_t *ctx, struct dsc$descriptor *qname_dsc, struct dsc$descriptor *msg_dsc)
{
    simple_string_t qname = desc2string(qname_dsc);
    simple_string_t msg = desc2string(msg_dsc);
    return simple_stomp_write(ctx, &qname, &msg);
}

int vms_simple_stomp_sub(simple_stomp_t *ctx, struct dsc$descriptor *qname_dsc)
{
    simple_string_t qname = desc2string(qname_dsc);
    return simple_stomp_sub(ctx, &qname);
}

int vms_simple_stomp_unsub(simple_stomp_t *ctx, struct dsc$descriptor *qname_dsc)
{
    simple_string_t qname = desc2string(qname_dsc);
    return simple_stomp_unsub(ctx, &qname);
}

int vms_simple_stomp_readone(simple_stomp_t *ctx, struct dsc$descriptor *msg_dsc, int *msglen)
{
    char *msg = msg_dsc->dsc$a_pointer;
    int status = simple_stomp_readone(ctx, msg, (size_t)*msglen);
    if (status == SIMPLE_STOMP_SUCCESS)
        *msglen = strlen(msg);
    return status;
}

int vms_simple_stomp_read(simple_stomp_t *ctx, struct dsc$descriptor *qname_dsc, struct dsc$descriptor *msg_dsc, int *msglen)
{
    char *msg = msg_dsc->dsc$a_pointer;
    simple_string_t qname = desc2string(qname_dsc);
    int status = simple_stomp_read(ctx, &qname, msg, (size_t)*msglen);
    if (status == SIMPLE_STOMP_SUCCESS)
        *msglen = strlen(msg);
    return status;
}

int vms_simple_stomp_close(simple_stomp_t *ctx)
{
    return simple_stomp_close(ctx);
}
