/*
   Copyright (C) CFEngine AS

   This file is part of CFEngine 3 - written and maintained by CFEngine AS.

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; version 3.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA

  To the extent this program is licensed as part of the Enterprise
  versions of CFEngine, the applicable Commerical Open Source License
  (COSL) may apply to this file if you as a licensee so wish it. See
  included file COSL.txt.
*/

#include "communication.h"

#include "alloc.h"                                      /* xmalloc,... */
#include "logging.h"                                    /* Log */
#include "misc_lib.h"                                   /* ProgrammingError */

AgentConnection *NewAgentConn(const char *server_name)
{
    AgentConnection *conn = xcalloc(1, sizeof(AgentConnection));

    conn->sd = SOCKET_INVALID;
    conn->family = AF_INET;
    conn->trust = false;
    conn->encryption_type = 'c';
    conn->this_server = xstrdup(server_name);
    return conn;
};

void DeleteAgentConn(AgentConnection *conn)
{
    Stat *sp = conn->cache;

    while (sp != NULL)
    {
        Stat *sps = sp;
        sp = sp->next;
        free(sps);
    }

    free(conn->session_key);
    free(conn->this_server);
    free(conn);
}

int IsIPV6Address(char *name)
{
    char *sp;
    int count, max = 0;

    if (name == NULL)
    {
        return false;
    }

    count = 0;

    for (sp = name; *sp != '\0'; sp++)
    {
        if (isalnum((int) *sp))
        {
            count++;
        }
        else if ((*sp != ':') && (*sp != '.'))
        {
            return false;
        }

        if (*sp == 'r')
        {
            return false;
        }

        if (count > max)
        {
            max = count;
        }
        else
        {
            count = 0;
        }
    }

    if (max <= 2)
    {
        return false;
    }

    if (strstr(name, ":") == NULL)
    {
        return false;
    }

    if (strcasestr(name, "scope"))
    {
        return false;
    }

    return true;
}

/*******************************************************************/

int IsIPV4Address(char *name)
{
    char *sp;
    int count = 0;

    if (name == NULL)
    {
        return false;
    }

    for (sp = name; *sp != '\0'; sp++)
    {
        if ((!isdigit((int) *sp)) && (*sp != '.'))
        {
            return false;
        }

        if (*sp == '.')
        {
            count++;
        }
    }

    if (count != 3)
    {
        return false;
    }

    return true;
}

/*****************************************************************************/

/**
 * @brief DNS lookup of hostname, store the address as string into dst of size
 * dst_size.
 * @return -1 in case of unresolvable hostname or other error.
 */
int Hostname2IPString(char *dst, const char *hostname, size_t dst_size)
{
    int ret;
    struct addrinfo *response, *ap;
    struct addrinfo query = {
        .ai_family = AF_UNSPEC,
        .ai_socktype = SOCK_STREAM
    };

    if (dst_size < CF_MAX_IP_LEN)
    {
        ProgrammingError("Hostname2IPString got %lu, needs at least"
                         " %d length buffer for IPv6 portability!",
                         dst_size, CF_MAX_IP_LEN);
    }

    ret = getaddrinfo(hostname, NULL, &query, &response);
    if ((ret) != 0)
    {
        Log(LOG_LEVEL_INFO,
            "Unable to lookup hostname '%s' or cfengine service. (getaddrinfo: %s)",
            hostname, gai_strerror(ret));
        return -1;
    }

    for (ap = response; ap != NULL; ap = ap->ai_next)
    {
        /* No lookup, just convert numeric IP to string. */
        int ret2 = getnameinfo(ap->ai_addr, ap->ai_addrlen,
                               dst, dst_size, NULL, 0, NI_NUMERICHOST);
        if (ret2 == 0)
        {
            freeaddrinfo(response);
            return 0;                                           /* Success */
        }
    }
    freeaddrinfo(response);

    Log(LOG_LEVEL_ERR,
        "Hostname2IPString: ERROR even though getaddrinfo returned success!");
    return -1;
}

/*****************************************************************************/

/**
 * @brief Reverse DNS lookup of ipaddr, store the address as string into dst
 * of size dst_size.
 * @return -1 in case of unresolvable IP address or other error.
 */
int IPString2Hostname(char *dst, const char *ipaddr, size_t dst_size)
{
    int ret;
    struct addrinfo *response;

    /* First convert ipaddr string to struct sockaddr, with no DNS query. */
    struct addrinfo query = {
        .ai_flags = AI_NUMERICHOST
    };

    ret = getaddrinfo(ipaddr, NULL, &query, &response);
    if (ret != 0)
    {
        Log(LOG_LEVEL_ERR,
            "Unable to convert IP address '%s'. (getaddrinfo: %s)",
              ipaddr, gai_strerror(ret));
        return -1;
    }

    /* response should only have one reply, so no need to iterate over the
     * response struct addrinfo. */

    /* Reverse DNS lookup. NI_NAMEREQD forces an error if not resolvable. */
    ret = getnameinfo(response->ai_addr, response->ai_addrlen,
                      dst, dst_size, NULL, 0, NI_NAMEREQD);
    if (ret != 0)
    {
        Log(LOG_LEVEL_INFO,
            "Couldn't reverse resolve '%s'. (getaddrinfo: %s)",
            ipaddr, gai_strerror(ret));
        freeaddrinfo(response);
        return -1;
    }

    freeaddrinfo(response);
    return 0;                                                   /* Success */
}

/*****************************************************************************/

int GetMyHostInfo(char nameBuf[MAXHOSTNAMELEN], char ipBuf[MAXIP4CHARLEN])
{
    char *ip;
    struct hostent *hostinfo;

    if (gethostname(nameBuf, MAXHOSTNAMELEN) == 0)
    {
        if ((hostinfo = gethostbyname(nameBuf)) != NULL)
        {
            ip = inet_ntoa(*(struct in_addr *) *hostinfo->h_addr_list);
            strncpy(ipBuf, ip, MAXIP4CHARLEN - 1);
            ipBuf[MAXIP4CHARLEN - 1] = '\0';
            return true;
        }
        else
        {
            Log(LOG_LEVEL_ERR, "Could not get host entry for local host. (gethostbyname: %s)", GetErrorStr());
        }
    }
    else
    {
        Log(LOG_LEVEL_ERR, "Could not get host name. (gethostname: %s)", GetErrorStr());
    }

    return false;
}

/*****************************************************************************/

unsigned short SocketFamily(int sd)
{
   struct sockaddr sa = {0};
   socklen_t len = sizeof(sa);

   if (getsockname(sd, &sa, &len) == -1)
   {
       Log(LOG_LEVEL_ERR, "Could not get socket family. (getsockname: %s)", GetErrorStr());
   }

   return sa.sa_family;
}
