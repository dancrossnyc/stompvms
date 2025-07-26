#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "vms_simple_stomp.h"

static void print(char *msg)
{
   printf("%s\n", msg);
}

static char *in_cstr(struct dsc$descriptor *s)
{
    int slen, ix;
    char *sptr, *res;
    if(s->dsc$b_class == DSC$K_CLASS_S || s->dsc$b_class == DSC$K_CLASS_D)
    {
        slen = s->dsc$w_length;
        sptr = s->dsc$a_pointer;
    }
    else if(s->dsc$b_class == DSC$K_CLASS_VS) 
    {
        slen = *((short int *)s->dsc$a_pointer);
        sptr = s->dsc$a_pointer + sizeof(short int);
    }
    else
    {
        fprintf(stderr, "Unsupported descriptor class: %d\n", s->dsc$b_class);
        exit(44);
    }
    res = malloc(slen + 1);
    strncpy(res, sptr, slen);
    res[slen] = 0;
    /* trim traling whitespace */
    ix = slen - 1;
    while(ix >= 0 && (res[ix] == ' ' || res[ix] == 0))
    {
        res[ix] = 0;
        ix--;
    }
    return res;
}

void vms_simple_stomp_debug(int *debug)
{
    simple_stomp_debug(*debug);
}


int vms_simple_stomp_init(simple_stomp_t *ctx, struct dsc$descriptor *host, int *port)
{
    int stat;
    char *host2;
    host2 = in_cstr(host);
    stat = simple_stomp_init(ctx, host2, *port, print);
    free(host2);
    return stat;
}

int vms_simple_stomp_write(simple_stomp_t *ctx, struct dsc$descriptor *qname, struct dsc$descriptor *msg)
{
    int stat;
    char *qname2, *msg2;
    qname2 = in_cstr(qname);
    msg2 = in_cstr(msg);
    stat = simple_stomp_write(ctx, qname2, msg2);
    free(qname2);
    free(msg2);
    return stat;
}

int vms_simple_stomp_sub(simple_stomp_t *ctx, struct dsc$descriptor *qname)
{
    int stat;
    char *qname2;
    qname2 = in_cstr(qname);
    stat = simple_stomp_sub(ctx, qname2);
    free(qname2);
    return stat;
}

int vms_simple_stomp_readone(simple_stomp_t *ctx, struct dsc$descriptor *msg, int *msglen)
{
    int stat;
    stat = simple_stomp_readone(ctx, msg->dsc$a_pointer);
    *msglen = strlen(msg->dsc$a_pointer);
    return stat;
}

int vms_simple_stomp_read(simple_stomp_t *ctx, struct dsc$descriptor *qname, struct dsc$descriptor *msg, int *msglen)
{
    int stat;
    char *qname2;
    qname2 = in_cstr(qname);
    stat = simple_stomp_read(ctx, qname2, msg->dsc$a_pointer);
    *msglen = strlen(msg->dsc$a_pointer);
    free(qname2);
    return stat;
}

int vms_simple_stomp_close(simple_stomp_t *ctx)
{
    return simple_stomp_close(ctx);
}
