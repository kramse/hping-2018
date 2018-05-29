/*
 * $smu-mark$
 * $name: sendip.c$
 * $author: Salvatore Sanfilippo <antirez@invece.org>$
 * $copyright: Copyright (C) 1999 by Salvatore Sanfilippo$
 * $license: This software is under GPL version 2 of license$
 * $date: Fri Nov  5 11:55:49 MET 1999$
 * $rev: 8$
 */

/* $Id: sendip.c,v 1.2 2004/04/09 23:38:56 antirez Exp $ */

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include "hping2.h"
#include "globals.h"

void send_ip (char* src, char *dst, char *data, unsigned int datalen,
		int more_fragments, unsigned short fragoff, char *options,
		char optlen)
{
	char		*packet, *vxpacket;
	int		result,
			packetsize,vxpacketsize;
	struct myiphdr	*ip,*vxip;
	int vxlanmode=1;
	struct myudphdr		*udp;
	struct myvxlanhdr		*vxlan;
	struct myvxetherhdr		*etherpacket;

	char vxdst_mac[6] = "AAAAAA";
	char vxsrc_mac[6] = "BBBBBB";

	vxpacketsize = IPHDR_SIZE *2 + UDPHDR_SIZE + optlen + datalen;

	packetsize = IPHDR_SIZE + optlen + datalen;
	if ( (packet = malloc(packetsize)) == NULL) {
		perror("[send_ip] malloc()");
		return;
	}

	memset(packet, 0, packetsize);
	ip = (struct myiphdr*) packet;

	/* copy src and dst address */
	memcpy(&ip->saddr, src, sizeof(ip->saddr));
	memcpy(&ip->daddr, dst, sizeof(ip->daddr));

	/* build ip header */
	ip->version	= 4;
	ip->ihl		= (IPHDR_SIZE + optlen + 3) >> 2;
	ip->tos		= ip_tos;

#if defined OSTYPE_DARWIN
	/* FreeBSD */
	/* NetBSD */
		ip->tot_len	= htons(packetsize);
#endif

#if defined OSTYPE_FREEBSD || defined OSTYPE_NETBSD || defined OSTYPE_BSDI
/* FreeBSD */
/* NetBSD */
	ip->tot_len	= packetsize;
#else
/* Linux */
/* OpenBSD */
	ip->tot_len	= htons(packetsize);
#endif

	if (!opt_fragment)
	{
		ip->id		= (src_id == -1) ?
			htons((unsigned short) rand()) :
			htons((unsigned short) src_id);
	}
	else /* if you need fragmentation id must not be randomic */
	{
		/* FIXME: when frag. enabled sendip_handler shold inc. ip->id */
		/*        for every frame sent */
		ip->id		= (src_id == -1) ?
			htons(getpid() & 255) :
			htons((unsigned short) src_id);
	}

#if defined OSTYPE_DARWIN || defined OSTYPE_FREEBSD || defined OSTYPE_NETBSD | defined OSTYPE_BSDI
/* FreeBSD */
/* NetBSD */
	ip->frag_off	|= more_fragments;
	ip->frag_off	|= fragoff >> 3;
#else
/* Linux */
/* OpenBSD */
	ip->frag_off	|= htons(more_fragments);
	ip->frag_off	|= htons(fragoff >> 3); /* shift three flags bit */
#endif

	ip->ttl		= src_ttl;
	if (opt_rawipmode)	ip->protocol = raw_ip_protocol;
	else if	(opt_icmpmode)	ip->protocol = 1;	/* icmp */
	else if (opt_udpmode)	ip->protocol = 17;	/* udp  */
	else			ip->protocol = 6;	/* tcp  */
	ip->check	= 0; /* always computed by the kernel */

	/* copies options */
	if (options != NULL)
		memcpy(packet+IPHDR_SIZE, options, optlen);

	/* Make it into a VXLAN packet, add IP/UDP */
	if (vxlanmode != 0) {
		vxpacketsize = packetsize + IPHDR_SIZE + UDPHDR_SIZE + VXLANHDR_SIZE + ETHERHDR_SIZE ;
		if ( (vxpacket = malloc(vxpacketsize)) == NULL) {
			perror("[send_ip] malloc() VXLAN");
			return;
		}
		memset(vxpacket, 0, packetsize);


		vxip = (struct myiphdr*) vxpacket;
		/* build VXLAN ip header */
		vxip->version	= 4;
		vxip->ihl		= (IPHDR_SIZE + optlen + 3) >> 2;
		vxip->tos		= ip_tos;
		/* copy src and dst address */
		memcpy(&vxip->saddr, src, sizeof(vxip->saddr));
		memcpy(&vxip->daddr, dst, sizeof(vxip->daddr));
		vxip->ttl		= src_ttl;
		if (!opt_fragment)
		{
			vxip->id		= (src_id == -1) ?
				htons((unsigned short) rand()) :
				htons((unsigned short) src_id);
		}
		#if defined OSTYPE_DARWIN || defined OSTYPE_FREEBSD || defined OSTYPE_NETBSD || defined OSTYPE_BSDI
		/* FreeBSD */
		/* NetBSD */
			vxip->tot_len	= vxpacketsize;
		#else
		/* Linux */
		/* OpenBSD */
			vxip->tot_len	= htons(vxpacketsize);
		#endif

		vxip->protocol = 17;	/* VXLAN is UDP  */
		udp =  (struct myudphdr*) (vxpacket+IPHDR_SIZE);
		/* udp header */
		udp->uh_dport	= htons(vxdst_port);
		udp->uh_sport	= htons(vxsrc_port);
		udp->uh_ulen	= htons(vxpacketsize-20);
//		udp->uh_sum = 0 ;  // Already zeroed, so skip

		vxlan =  (struct myvxlanhdr*) (vxpacket+IPHDR_SIZE+UDPHDR_SIZE);
		vxlan->flags	= 8;
		vxlan->vni_high	= vxvni / 65536;
		vxlan->vni_med	= (vxvni - vxlan->vni_high * 65536 ) / 256;
		vxlan->vni_low	= vxvni - vxlan->vni_med * 256 - vxlan->vni_high * 65536;
//		vxlan->vni_low	= vxvni;

		// Only IPv4 right now, type 0x0800, IPv6 would bx 0x86DD
		etherpacket =  (struct myvxetherhdr*) (vxpacket+IPHDR_SIZE+UDPHDR_SIZE+VXLANHDR_SIZE);
		memcpy(&etherpacket->vxdst_mac, vxdst_mac, 6);
		memcpy(&etherpacket->vxsrc_mac, vxsrc_mac, 6);
		etherpacket->type	= htons(0x0800);

		memcpy(packet + IPHDR_SIZE + optlen, data, datalen);
		memcpy(vxpacket + IPHDR_SIZE + UDPHDR_SIZE + VXLANHDR_SIZE + ETHERHDR_SIZE , packet, packetsize);

		if (opt_debug == TRUE) {
			unsigned int i;
			for (i=0; i<packetsize; i++)
				printf("%.2X ", packet[i]&255);
				printf("\n");
		}
		free(packet);
		packet = vxpacket;
		if (opt_debug == TRUE)
			{
					unsigned int i;

					for (i=0; i<vxpacketsize; i++)
							printf("%.2X ", vxpacket[i]&255);
					printf("\n");
			}
	}
	else {
		/* copies data */
		memcpy(packet + IPHDR_SIZE + optlen, data, datalen);
	}

	result = sendto(sockraw, vxpacket, vxpacketsize, 0,
		(struct sockaddr*)&remote, sizeof(remote));

	if (result == -1 && errno != EINTR && !opt_rand_dest && !opt_rand_source) {
		perror("[send_ip] sendto");
		if (close(sockraw) == -1)
			perror("[ipsender] close(sockraw)");
		if (close_pcap() == -1)
			printf("[ipsender] close_pcap failed\n");
		exit(1);
	}

	free(packet);

	/* inc packet id for safe protocol */
	if (opt_safe && !eof_reached)
		src_id++;
}
