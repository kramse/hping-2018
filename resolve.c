/*
 * $smu-mark$
 * $name: resolve.c$
 * $author: Salvatore Sanfilippo <antirez@invece.org>$
 * $copyright: Copyright (C) 1999 by Salvatore Sanfilippo$
 * $license: This software is under GPL version 2 of license$
 * $date: Fri Nov  5 11:55:49 MET 1999$
 * $rev: 8$
 */

/* $Id: resolve.c,v 1.2 2003/09/01 00:22:06 antirez Exp $ */

#include <stdlib.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

/* On error -1 is returned, on success 0 */
int resolve_addr(struct sockaddr * addr, char *hostname)
{
	struct  sockaddr_in *address;
	struct  hostent     *host;

	address = (struct sockaddr_in *)addr;

	memset(address, 0, sizeof(struct sockaddr_in));
	address->sin_family = AF_INET;
	address->sin_addr.s_addr = inet_addr(hostname);

	if ( (int)address->sin_addr.s_addr == -1) {
		host = gethostbyname(hostname);
		if (host) {
			memcpy(&address->sin_addr, host->h_addr,
				host->h_length);
			return 0;
		} else {
			return -1;
		}
	}
	return 0;
}

/* On error -1 is returned, on success 0 */
int resolve_addr6(struct sockaddr * addr, char *hostname)
{
	struct sockaddr_in6 *address;
	struct addrinfo hints, *res;
	int err;

	address = (struct sockaddr_in6 *)addr;
	memset(address, 0, sizeof(struct sockaddr_in6));
	address->sin6_family = AF_INET6;

	memset (&hints, 0, sizeof (hints));
	/* We only care about IPV6 results */
	hints.ai_family = AF_INET6;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = 0;


  // look up
  err = getaddrinfo(hostname, NULL, &hints, &res);
  if(err != 0) {
      printf("ERR: %s\n", gai_strerror(err));
      return err;
  }

	while (res)
	  {
	    /* Check to make sure we have a valid AF_INET6 address */
	    if(res->ai_family == AF_INET6) {
					/* Use memcpy since we're going to free the res variable later */
	          memcpy (address, res->ai_addr, res->ai_addrlen);
	          break;
	         }

	         res = res->ai_next;
	     }
    // don't forget to free the addrinfo
    freeaddrinfo(res);
	return 0;
}


/* Like resolve_addr but exit on error */
void resolve(struct sockaddr *addr, char *hostname)
{
	if (resolve_addr(addr, hostname) == -1) {
		fprintf(stderr, "Unable to resolve '%s'\n", hostname);
		exit(1);
	}
}

/* Like resolve_addr but exit on error */
void resolve6(struct sockaddr *addr, char *hostname)
{
	if (resolve_addr6(addr, hostname) == -1) {
		fprintf(stderr, "Unable to resolve '%s'\n", hostname);
		exit(1);
	}
}
