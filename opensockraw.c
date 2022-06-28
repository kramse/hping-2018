/*
 * $smu-mark$
 * $name: opensockraw.c$
 * $author: Salvatore Sanfilippo <antirez@invece.org>$
 * $copyright: Copyright (C) 1999 by Salvatore Sanfilippo$
 * $license: This software is under GPL version 2 of license$
 * $date: Fri Nov  5 11:55:49 MET 1999$
 * $rev: 8$
 */

/* $Id: opensockraw.c,v 1.2 2003/09/01 00:22:06 antirez Exp $ */

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> /* IPPROTO_RAW def. */
#include <linux/if_ether.h>


#include "hping2.h"
#include "globals.h"

int open_sockraw()
{
	int s;

	if (! opt_inet6mode ) {
		s = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
		if (s == -1) {
			perror("[open_sockraw] socket()");
			return -1;
		}
	} else {
		// Input from https://blog.apnic.net/2017/10/24/raw-sockets-ipv6/
		// AF_INET6 raw is not completely raw, may mess with packets!
		s = socket (PF_PACKET, SOCK_RAW, htons (ETH_P_ALL));
		if (s < 0 ) {
			perror("[open_sockraw] socket()");
			return -1;
		}
	}

	return s;
}
