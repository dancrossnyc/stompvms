#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "simple_stomp.h"

static int PRINT_DEBUG = 0;

#define MAKE_STRING(S) (simple_string_t){ .s = S, .len = sizeof(S) - 1, }

typedef struct Header Header;
struct Header {
    const simple_string_t name;
    const simple_string_t *data;
};

typedef struct Frame Frame;
struct Frame {
    char buffer[MAX_BUF];
    size_t size;
    const char *cmd;
    const char *headers;
    const char *body;
    size_t bodysize;
};

static simple_stomp_result_t
copy(char **dst, const char *src, const size_t srclen, size_t *size)
{
    char *p;

    if (srclen >= *size)
        return SIMPLE_STOMP_TRUNCATED;
    p = *dst;
    memcpy(p, src, srclen);
    p[srclen] = '\0';
    *size -= srclen;
    *dst += srclen;

    return SIMPLE_STOMP_SUCCESS;
}

static simple_stomp_result_t
copyline(char **dst, const char *src, const size_t srclen, size_t *size)
{
    simple_stomp_result_t err;
    if ((err = copy(dst, src, srclen, size)) != SIMPLE_STOMP_SUCCESS)
        return err;
    return copy(dst, "\n", 1, size);
}

static inline void
xdebug(const char *s)
{
    const int XDEBUG_PRINT =
#ifndef XDEBUG
        0
#else
        1
#endif
        ;
    if (XDEBUG_PRINT) {
        do {
            if (*s == '\0')
                putchar('0');
            else if (*s == '\n')
                putchar('#');
            else
                putchar(*s);
        } while (*s++ != '\0');
    putchar('\n');
    }
}

static inline void
debug(const char *s)
{
    if (PRINT_DEBUG)
        printf("%s\n", s);
    xdebug(s);
}


static simple_stomp_result_t
makeframe(Frame *dst,
    const simple_string_t cmd,
    const Header *headers, const size_t nheaders,
    const simple_string_t *body)
{
    char *p = dst->buffer;
    size_t size = sizeof(dst->buffer);
    simple_stomp_result_t err;

    dst->cmd = p;
    if ((err = copyline(&p, cmd.s, cmd.len, &size)) != SIMPLE_STOMP_SUCCESS)
        return err;

    if (nheaders != 0)
        dst->headers = p;
    for (size_t i = 0; i < nheaders; i++) {
        const Header *h = headers + i;
        if (h->data == NULL || h->data->len == 0)
            continue;
        if ((err = copy(&p, h->name.s, h->name.len, &size)) != SIMPLE_STOMP_SUCCESS)
            return err;
        if ((err = copy(&p, ":", 1, &size)) != SIMPLE_STOMP_SUCCESS)
            return err;
        if ((err = copyline(&p, h->data->s, h->data->len, &size)) != SIMPLE_STOMP_SUCCESS)
            return err;
    }
    if ((err = copyline(&p, "", 0, &size)) != SIMPLE_STOMP_SUCCESS)
        return err;
    if (body != NULL) {
        dst->body = p;
        if ((err = copy(&p, body->s, body->len, &size)) != SIMPLE_STOMP_SUCCESS)
            return err;
    }
    dst->size = MAX_BUF - size + 1;
    debug(dst->buffer);

    return SIMPLE_STOMP_SUCCESS;
}

static void
reporterror(simple_stomp_t *ctx, int serrno, const char *fmt, ...)
{
    va_list args;
    char *dst;
    size_t size;
    size_t len;

    dst = ctx->errmsg;
    size = sizeof(ctx->errmsg);
    va_start(args, fmt);
    len = (size_t)vsnprintf(dst, size, fmt, args);
    va_end(args);
    if (serrno != 0 && len < size)
        snprintf(dst + len, size - len, ": %s", strerror(serrno));
    ctx->err_handler(ctx);
}

static simple_stomp_result_t
send_frame(simple_stomp_t *ctx, const Frame *frame)
{
    size_t size = frame->size;
    const char *buffer = frame->buffer;
    ssize_t len;

    while (size > 0) {
        len = send(ctx->sd, buffer, size, 0);
        if (len < 0) {
            reporterror(ctx, errno, "Error sending to socket");
            return SIMPLE_STOMP_FAILURE;
        }
        buffer += len;
        size -= len;
    }

    return SIMPLE_STOMP_SUCCESS;
}

static simple_stomp_result_t
get_raw_frame(simple_stomp_t *ctx, Frame *frame)
{
    ssize_t len;
    char *end;

    while ((end = memchr(ctx->recvbuf, 0, ctx->recvsize)) == NULL) {
        len = recv(ctx->sd, ctx->recvbuf + ctx->recvsize, sizeof(ctx->recvbuf) - ctx->recvsize, 0);
        if (len < 0) {
            reporterror(ctx, errno, "recv from network failed");
            return SIMPLE_STOMP_FAILURE;
        }
        ctx->recvsize += len;
        if (ctx->recvsize == sizeof(ctx->recvbuf)) {
            reporterror(ctx, 0, "frame truncated");
            return SIMPLE_STOMP_TRUNCATED;
        }
    }
    size_t framelen = end - ctx->recvbuf + 1;
    memcpy(frame->buffer, ctx->recvbuf, framelen);
    memmove(ctx->recvbuf, end + 1, sizeof(ctx->recvbuf) - framelen);
    ctx->recvsize -= framelen;

    return SIMPLE_STOMP_SUCCESS;
}

static simple_stomp_result_t
recv_frame(simple_stomp_t *ctx, Frame *frame)
{
    char *s, *p;
    simple_stomp_result_t err;

    if ((err = get_raw_frame(ctx, frame)) != SIMPLE_STOMP_SUCCESS) {
        return err;
    }
    debug(frame->buffer);

    s = p = frame->buffer;
    p = strchr(s, '\n');
    if(p == NULL) {
        reporterror(ctx, 0, "No command in frame");
        return SIMPLE_STOMP_FAILURE;
    }
    frame->cmd = s;
    *p++ = '\0';
    s = p;
    if (*s != '\n') {
        do {
            p = strchr(p, '\n');
            if (p == NULL) {
                reporterror(ctx, 0, "No body in frame");
                return SIMPLE_STOMP_FAILURE;
            }
            p++;
        } while (*p != '\n');
    }
    frame->headers = s;
    *p++ = '\0';
    frame->body = p;
    frame->bodysize = frame->size - (p - frame->buffer);

    return SIMPLE_STOMP_SUCCESS;
}

void
simple_stomp_debug(int debug)
{
    PRINT_DEBUG = debug;
}

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

simple_stomp_result_t
simple_stomp_init(simple_stomp_t *ctx,
    const simple_string_t *host, int port,
    const simple_string_t *client_id,
    const simple_string_t *username,
    const simple_string_t *password,
    error_handler eh)
{
    Frame frame;
    struct sockaddr_in remote;
    struct hostent *hostinfo;
    char hostname[256];
    const simple_string_t accepted_vers_data = MAKE_STRING("1.0,1.1,1.2");
    const Header headers[] = {
        { .name = MAKE_STRING("accept-version"), .data = &accepted_vers_data },
        { .name = MAKE_STRING("client-id"), .data = client_id },
        { .name = MAKE_STRING("login"), .data = username },
        { .name = MAKE_STRING("passcode"), .data = password },
    };

    memset(ctx->recvbuf, 0xFF, sizeof(ctx->recvbuf));
    ctx->recvsize = 0;
    ctx->err_handler = eh;

    ctx->sd = socket(AF_INET,SOCK_STREAM, 0);
    if (ctx->sd < 0) {
        reporterror(ctx, errno, "Error creating socket");
        return SIMPLE_STOMP_FAILURE;
    }

    if (host->len >= sizeof(hostname)) {
        reporterror(ctx, 0, "hostname too long");
        close(ctx->sd);
        return SIMPLE_STOMP_TRUNCATED;
    }
    memcpy(hostname, host->s, host->len);
    hostname[host->len] = '\0';
    hostinfo = gethostbyname(hostname);
    if (hostinfo == NULL) {
        reporterror(ctx, errno, "Hostname '%s' not found", host);
        close(ctx->sd);
        return SIMPLE_STOMP_FAILURE;
    }

    memset(&remote, 0, sizeof(remote));
    remote.sin_family = hostinfo->h_addrtype;
    remote.sin_port = htons(port);
    memcpy(&remote.sin_addr, hostinfo->h_addr, hostinfo->h_length);
    if (connect(ctx->sd, (struct sockaddr *)&remote, sizeof(remote)) < 0) {
        reporterror(ctx, errno, "Error connecting to host %s port %d", host, port);
        close(ctx->sd);
        return SIMPLE_STOMP_FAILURE;
    }
    makeframe(&frame, MAKE_STRING("CONNECT"), headers, ARRAY_SIZE(headers), NULL);
    if (send_frame(ctx, &frame) != SIMPLE_STOMP_SUCCESS) {
        reporterror(ctx, 0, "Error sending CONNECT");
        close(ctx->sd);
        return SIMPLE_STOMP_FAILURE;
    }
    if (recv_frame(ctx, &frame) != SIMPLE_STOMP_SUCCESS) {
        reporterror(ctx, 0, "Error receiving CONNECTED frame");
        close(ctx->sd);
        return SIMPLE_STOMP_FAILURE;
    }
    if (strcmp(frame.cmd, "CONNECTED") != 0) {
        reporterror(ctx, 0, "Unexpected frame received: %s", frame.cmd);
        close(ctx->sd);
        return SIMPLE_STOMP_FAILURE;
    }

    return SIMPLE_STOMP_SUCCESS;
}

simple_stomp_result_t
simple_stomp_write(simple_stomp_t *ctx, const simple_string_t *qname, const simple_string_t *msg)
{
    const Header destination = { .name = MAKE_STRING("destination"), .data = qname };
    Frame frame;

    if (makeframe(&frame, MAKE_STRING("SEND"), &destination, 1, msg) != SIMPLE_STOMP_SUCCESS) {
        reporterror(ctx, 0, "Error creating frame for SEND");
        return SIMPLE_STOMP_FAILURE;
    }
    if (send_frame(ctx, &frame) != SIMPLE_STOMP_SUCCESS) {
        reporterror(ctx, errno, "Error sending SEND");
        return SIMPLE_STOMP_FAILURE;
    }

    return SIMPLE_STOMP_SUCCESS;
}

simple_stomp_result_t
simple_stomp_sub(simple_stomp_t *ctx, const simple_string_t *qname)
{
    const Header destination = { .name = MAKE_STRING("destination"), .data = qname };
    Frame frame;

    makeframe(&frame, MAKE_STRING("SEND"), &destination, 1, NULL);
    if (send_frame(ctx, &frame) != SIMPLE_STOMP_SUCCESS) {
        reporterror(ctx, 0, "Error sending SUBSCRIBE");
        return SIMPLE_STOMP_FAILURE;
    }

    return SIMPLE_STOMP_SUCCESS;
}

simple_stomp_result_t
simple_stomp_unsub(simple_stomp_t *ctx, const simple_string_t *queueid)
{
    const Header queue = { .name = MAKE_STRING("id"), .data = queueid};
    const Header *qp = queueid != NULL ? &queue : NULL;
    size_t nheader = queueid != NULL;
    Frame frame;

    if (makeframe(&frame, MAKE_STRING("UNSUBSCRIBE"), qp, nheader, NULL) != SIMPLE_STOMP_SUCCESS) {
        reporterror(ctx, 0, "Error creating UNSUBSCRIBE");
        return SIMPLE_STOMP_FAILURE;
    }
    if (send_frame(ctx, &frame) != SIMPLE_STOMP_SUCCESS) {
        reporterror(ctx, errno, "Error sending UNSUBSCRIBE");
        return SIMPLE_STOMP_FAILURE;
    }

    return SIMPLE_STOMP_SUCCESS;
}

simple_stomp_result_t
simple_stomp_readone(simple_stomp_t *ctx, void *msg, size_t msgsize)
{
    Frame frame;

    if (recv_frame(ctx, &frame) != SIMPLE_STOMP_SUCCESS) {
        reporterror(ctx, 0, "Error receiving MESSAGE");
        return SIMPLE_STOMP_FAILURE;
    }
    if (strcmp(frame.cmd, "MESSAGE") != 0) {
        reporterror(ctx, 0, "Unexpected frame received: %s", frame.cmd);
        return SIMPLE_STOMP_FAILURE;
    }
    if (msgsize > frame.bodysize)
        return SIMPLE_STOMP_TRUNCATED;
    memcpy(msg, frame.body, frame.bodysize);

    return SIMPLE_STOMP_SUCCESS;
}

simple_stomp_result_t
simple_stomp_read(simple_stomp_t *ctx, const simple_string_t *qname, void *msg, size_t msgsize)
{
    simple_stomp_result_t err = simple_stomp_sub(ctx, qname);
    if (err != SIMPLE_STOMP_SUCCESS)
        return err;
    return simple_stomp_readone(ctx, msg, msgsize);
}

simple_stomp_result_t
simple_stomp_close(simple_stomp_t *ctx)
{
    Frame frame;

    makeframe(&frame, MAKE_STRING("DISCONNECT"), NULL, 0, NULL);
    if (send_frame(ctx, &frame) != SIMPLE_STOMP_SUCCESS) {
        reporterror(ctx, 0, "Error sending CONNECT");
        return SIMPLE_STOMP_FAILURE;
    }
    close(ctx->sd);
    return SIMPLE_STOMP_SUCCESS;
}
