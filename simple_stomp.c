#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <sys/socket.h>
#include <fcntl.h>
#include <netdb.h>
#include <errno.h>

#include "simple_stomp.h"

static int print_debug = 0;

static int helper_write(simple_stomp_t *ctx, const char *cmd, const char *header, const char *body)
{
   int status;
   char buf[MAX_BUF], errmsg[200], *p;
   p = buf;
   p += snprintf(p, sizeof(buf) - (p - buf), "%s\n", cmd);
   if(header != NULL)
   {
      p += snprintf(p, sizeof(buf) - (p - buf), "%s\n", header);
   }
   p += snprintf(p, sizeof(buf) - (p - buf), "\n");
   if(body != NULL)
   {
      p += snprintf(p, sizeof(buf) - (p - buf), "%s", body);
   }
   if(print_debug)
   {
      printf("%s\n", buf);
   }
#ifdef XDEBUG
    for(int i = 0; i < strlen(buf) + 1; i++)
    {
        switch(buf[i]) {
            case 0:
                putchar('0');
                break;
            case '\n':
                putchar('#');
                break;
            default:
                putchar(buf[i]);
                break;
         }
    }
    putchar('\n');
#endif
   status = send(ctx->sd, buf, strlen(buf) + 1, 0);
   if(status < 0)
   {
      snprintf(errmsg, sizeof(errmsg), "Error sending to socket: %s", strerror(errno));
      ctx->eh(errmsg);
      return 0;
   }
   return 1;
}

static int helper_read(simple_stomp_t *ctx, char *cmd, char *header, char *body)
{
   char errmsg[200], *p;
   while(ctx->ix < sizeof(ctx->buf) && ctx->ix <= strlen(ctx->buf))
   {
      ctx->ix += recv(ctx->sd, ctx->buf + ctx->ix, sizeof(ctx->buf) - ctx->ix, 0);
   }
   if(print_debug)
   {
      printf("%s\n", ctx->buf);
   }
#ifdef XDEBUG
    for(int i = 0; i < ctx->ix; i++)
    {
        switch(ctx->buf[i]) {
            case 0:
                putchar('0');
                break;
            case '\n':
                putchar('#');
                break;
            default:
                putchar(ctx->buf[i]);
                break;
        }
    }
    putchar('\n');
#endif
   p = strstr(ctx->buf, "\n");
   if(p != NULL)
   {
      strncpy(cmd, ctx->buf, p - ctx->buf);
      cmd[p - ctx->buf] = '\0';
   }
   else
   {
      ctx->eh("No command found in frame");
      return 0;
   }
   p = strstr(ctx->buf, "\n\n");
   if(p != NULL)
   {
      strcpy(body, p + 2);
   }
   else
   {
      ctx->eh("No body found in frame");
      return 0;
   }
   int len = strlen(ctx->buf) + 2;
   memmove(ctx->buf, ctx->buf + len, ctx->ix - len);
   ctx->ix -= len;
   ctx->buf[ctx->ix] = 0;
#ifdef XDEBUG
    putchar('>');
    for(int i = 0; i < ctx->ix; i++)
    {
        switch(ctx->buf[i]) {
            case 0:
                putchar('0');
                break;
            case '\n':
                putchar('#');
                break;
            default:
                putchar(ctx->buf[i]);
                break;
        }
    }
    putchar('\n');
#endif
   return 1;
}

void simple_stomp_debug(int debug)
{
   print_debug = debug;
}

int simple_stomp_initx(simple_stomp_t *ctx, const char *host, int port,
                       const char *clientid, const char *un, const char *pw,
                       error_handler eh)
{
   int status;
   char errmsg[200], cmd[800], header[800], body[800];
   struct sockaddr local, remote;
   struct hostent *hostinfo;
   ctx->eh = eh;
   ctx->sd = socket(AF_INET,SOCK_STREAM, 0);
   if(ctx->sd < 0)
   {
      snprintf(errmsg, sizeof(errmsg), "Error creating socket: %s", strerror(errno));
      ctx->eh(errmsg);
      return 0;
   }
   local.sa_family = AF_INET;
   memset(local.sa_data, 0, sizeof(local.sa_data));
   status = bind(ctx->sd, &local, sizeof(local));
   if(status<0)
   {
      snprintf(errmsg, sizeof(errmsg), "Error binding socket: %s", strerror(errno));
      ctx->eh(errmsg);
      return 0;
   }
   hostinfo = gethostbyname(host);
   if(!hostinfo)
   {
      snprintf(errmsg, sizeof(errmsg), "Error looking up host %s: %s", host, strerror(errno));
      ctx->eh(errmsg);
      return 0;
   }
   remote.sa_family = hostinfo->h_addrtype;
   memcpy(remote.sa_data + 2, hostinfo->h_addr_list[0], hostinfo->h_length);
   *((short *)remote.sa_data) = htons(port);
   status = connect(ctx->sd, &remote, sizeof(remote));
   if(status!=0) {
      snprintf(errmsg, sizeof(errmsg), "Error connecting to host %s port %d: %s", host, port, strerror(errno));
      ctx->eh(errmsg);
      return 0;
   }
   ctx->ix = 0;
   ctx->buf[ctx->ix] = 0;
   if(clientid == NULL)
   {
       clientid = getenv("clientid");
   }
   if(clientid == NULL && un == NULL & pw == NULL)
   {
       if(!helper_write(ctx, "CONNECT", NULL, NULL))
       {
          snprintf(errmsg, sizeof(errmsg), "Error sending CONNECT");
          ctx->eh(errmsg);
          return 0;
       }
   }
   else
   {
       char header[800];
       char *p = header;
       p += snprintf(p, sizeof(header) - (p - header), "accept-version:1.0,1.1,1.2");
       if(clientid != NULL)
       {
           p += snprintf(p, sizeof(header) - (p - header), "\nclient-id:%s", clientid);
       }
       if(un != NULL)
       {
           p += snprintf(p, sizeof(header) - (p - header), "\nlogin:%s", un);
       }
       if(pw != NULL)
       {
           p += snprintf(p, sizeof(header) - (p - header), "\npasscode:%s", pw);
       }
       if(!helper_write(ctx, "CONNECT", header, NULL))
       {
          snprintf(errmsg, sizeof(errmsg), "Error sending CONNECT");
          ctx->eh(errmsg);
          return 0;
       }
   }
   if(!helper_read(ctx, cmd, header, body))
   {
      snprintf(errmsg, sizeof(errmsg), "Error receiving CONNECTED");
      ctx->eh(errmsg);
      return 0;
   }
   if(strcmp(cmd, "CONNECTED") != 0)
   {
       ctx->eh(body);
       return 0;
   }
   return 1;
}

int simple_stomp_init(simple_stomp_t *ctx, const char *host, int port, error_handler eh)
{
    return simple_stomp_initx(ctx, host, port, NULL, NULL, NULL, eh);
}

int simple_stomp_write(simple_stomp_t *ctx, const char *qname, const char *msg)
{
   char header[800], errmsg[200];
   snprintf(header, sizeof(header), "destination: %s", qname);
   if(!helper_write(ctx, "SEND", header, msg))
   {
      snprintf(errmsg, sizeof(errmsg), "Error sending SEND");
      ctx->eh(errmsg);
      return 0;
   }
   return 1;
}

int simple_stomp_sub(simple_stomp_t *ctx, const char *qname)
{
   char cmd[800], header[800], errmsg[200];
   snprintf(header, sizeof(header), "destination: %s", qname);
   if(!helper_write(ctx, "SUBSCRIBE", header, NULL))
   {
      snprintf(errmsg, sizeof(errmsg), "Error sending SUBSCRIBE");
      ctx->eh(errmsg);
      return 0;
   }
   return 1;
}

int simple_stomp_unsub(simple_stomp_t *ctx, const char *qname)
{
   char cmd[800], header[800], errmsg[200];
   char *p = strstr(qname, "\n");
   if(p == NULL)
   {
       if(!helper_write(ctx, "UNSUBSCRIBE", NULL, NULL))
       {
          snprintf(errmsg, sizeof(errmsg), "Error sending UNSUBSCRIBE");
          ctx->eh(errmsg);
          return 0;
       }
   }
   else
   {
       strcpy(header, p + 1);
       if(!helper_write(ctx, "UNSUBSCRIBE", header, NULL))
       {
          snprintf(errmsg, sizeof(errmsg), "Error sending UNSUBSCRIBE");
          ctx->eh(errmsg);
          return 0;
       }
   }
   return 1;
}

int simple_stomp_readone(simple_stomp_t *ctx, char *msg)
{
   char cmd[800], header[800], errmsg[200];
   if(!helper_read(ctx, cmd, header, msg))
   {
      snprintf(errmsg, sizeof(errmsg), "Error receiving MESSAGE");
      ctx->eh(errmsg);
      return 0;
   }
   return 1;
}

int simple_stomp_read(simple_stomp_t *ctx, const char *qname, char *msg)
{
    int stat = simple_stomp_sub(ctx, qname);
    if(stat != 0) return stat;
    return simple_stomp_readone(ctx, msg);
}

int simple_stomp_close(simple_stomp_t *ctx)
{
   char errmsg[200];
   if(!helper_write(ctx, "DISCONNECT", NULL, NULL))
   {
      snprintf(errmsg, sizeof(errmsg), "Error sending CONNECT");
      ctx->eh(errmsg);
      return 0;
   }
   close(ctx->sd);
   return 1;
}

