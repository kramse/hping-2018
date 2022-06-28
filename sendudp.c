/*
 * $smu-mark$
 * $name: sendudp.c$
 * $author: Salvatore Sanfilippo <antirez@invece.org>$
 * $copyright: Copyright (C) 1999 by Salvatore Sanfilippo$
 * $license: This software is under GPL version 2 of license$
 * $date: Fri Nov  5 11:55:49 MET 1999$
 * $rev: 8$
 */

/* $Id: sendudp.c,v 1.2 2003/09/01 00:22:06 antirez Exp $ */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>

#include "hping2.h"
#include "globals.h"

/* void hexdumper(unsigned char *packet, int size); */

void send_udp(void)
{
	int			packet_size;
	char			*packet, *data;
	struct myudphdr		*udp;
	struct pseudohdr *pseudoheader;
	struct pseudohdr6	*pseudoheader6;

	packet_size = UDPHDR_SIZE + data_size;

	if ( !opt_inet6mode ) {
	packet = malloc(PSEUDOHDR_SIZE + packet_size);
	if (packet == NULL) {
		perror("[send_udphdr] malloc()");
		return;
	}
	pseudoheader = (struct pseudohdr*) packet;
	udp =  (struct myudphdr*) (packet+PSEUDOHDR_SIZE);
	data = (char*) (packet+PSEUDOHDR_SIZE+UDPHDR_SIZE);

	memset(packet, 0, PSEUDOHDR_SIZE+packet_size);

	/* udp pseudo header */
	memcpy(&pseudoheader->saddr, &local.sin_addr.s_addr, 4);
	memcpy(&pseudoheader->daddr, &remote.sin_addr.s_addr, 4);
	pseudoheader->protocol		= 17; /* udp */
	pseudoheader->lenght		= htons(packet_size);

	} else {
	// IPv6
	packet = malloc(PSEUDOHDR6_SIZE + packet_size);
	if (packet == NULL) {
		perror("[send_udphdr] malloc()");
		return;
	}
	pseudoheader6 = (struct pseudohdr6*) packet;
	udp =  (struct myudphdr*) (packet+PSEUDOHDR6_SIZE);
	data = (char*) (packet+PSEUDOHDR6_SIZE+UDPHDR_SIZE);

	memset(packet, 0, PSEUDOHDR6_SIZE+packet_size);

	/* udp pseudo header */
	memcpy(&pseudoheader6->saddr, &local6.sin6_addr, 16);
	memcpy(&pseudoheader6->daddr, &remote6.sin6_addr, 16);
	pseudoheader6->protocol		= 17; /* udp */
	pseudoheader6->lenght		= htons(packet_size);
	}

	/* udp header */
	udp->uh_dport	= htons(dst_port);
	udp->uh_sport	= htons(src_port);
	udp->uh_ulen	= htons(packet_size);

	/* data */
	data_handler(data, data_size);

	/* compute checksum */
	if ( ! opt_inet6mode ) {
#ifdef STUPID_SOLARIS_CHECKSUM_BUG
	udp->uh_sum = packet_size;
#else
	udp->uh_sum = cksum((__u16*) packet, PSEUDOHDR_SIZE +
		      packet_size);
#endif
	} else {
		udp->uh_sum = cksum((__u16*) packet, PSEUDOHDR6_SIZE +
					packet_size);
	}

	/* adds this pkt in delaytable */
	delaytable_add(sequence, src_port, time(NULL), get_usec(), S_SENT);

	if (opt_debug == TRUE) {
		unsigned int i;
		printf("[send_udp] packet debug with pseudo header %d\n", packet_size);
		for (i=0; i<(PSEUDOHDR6_SIZE+packet_size); i++)
			printf("%.2X ", packet[i]&255);
		printf("\n");
	}

	/* send packet */
	if ( ! opt_inet6mode ) {
		send_ip_handler(packet+PSEUDOHDR_SIZE, packet_size);
	} else {
		send_ip_handler(packet+PSEUDOHDR6_SIZE, packet_size);
	}
	free(packet);

	sequence++;	/* next sequence number */

	if (!opt_keepstill)
		src_port = (sequence + initsport) % 65536;

	if (opt_force_incdport)
		dst_port++;
}
