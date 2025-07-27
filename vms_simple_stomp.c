#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <ssdef.h>    // VMS specific

#include "vms_simple_stomp.h"

static void
vms_simple_stomp_print_err(const simple_stomp_t *ctx)
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
        exit(SS$ABORT);
    }

    return (simple_string_t){ .s = s, .len = len };
}

void vms_simple_stomp_debug(int *debug)
{
    simple_stomp_debug(*debug);
}


int vms_simple_stomp_initx(simple_stomp_t *ctx,
    struct dsc$descriptor *host_dsc, int *pport,
    struct dsc$descriptor *client_id_dsc,
    struct dsc$descriptor *username_dsc,
    struct dsc$descriptor *password_dsc)
{
    simple_string_t host = desc2string(host_dsc);
    simple_string_t clientid, *clientidp = NULL;
    simple_string_t username, *usernamep = NULL;
    simple_string_t password, *passwordp = NULL;
    if (client_id_dsc != NULL) {
        clientid = desc2string(client_id_dsc);
        clientidp = &clientid;
    } else {
        char *cid = getenv("clientid");
        if (cid != NULL) {
            clientid = (simple_string_t){
                .s = cid,
                .len = strlen(cid),
            };
            clientidp = &clientid;
        }
    }
    if (username_dsc != NULL) {
        username = desc2string(username_dsc);
        usernamep = &username;
    }
    if (password_dsc != NULL) {
        password = desc2string(password_dsc);
        passwordp = &password;
    }
    return simple_stomp_init(ctx, &host, *pport,
        clientidp, usernamep, passwordp,
        vms_simple_stomp_print_err);
}

int vms_simple_stomp_write(simple_stomp_t *ctx,
    struct dsc$descriptor *qname_dsc, struct dsc$descriptor *msg_dsc)
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

int vms_simple_stomp_readone(simple_stomp_t *ctx,
    struct dsc$descriptor *msg_dsc, int *msglen)
{
    char *msg = msg_dsc->dsc$a_pointer;
    int status = simple_stomp_readone(ctx, msg, (size_t)*msglen);
    if (status == SIMPLE_STOMP_SUCCESS)
        *msglen = strlen(msg);
    return status;
}

int vms_simple_stomp_read(simple_stomp_t *ctx,
    struct dsc$descriptor *qname_dsc,
    struct dsc$descriptor *msg_dsc, int *msglen)
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
