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
#include <linux/if_packet.h> /* struct sockaddr_ll */

#include "hping2.h"
#include "globals.h"

void send_ip (char* src, char *dst, char *data, unsigned int datalen,
		int more_fragments, unsigned short fragoff, char *options,
		char optlen)
{
	char	*ether_frame, *packet, *vxpacket;
	int		result,
			packetsize,vxpacketsize;
	struct myiphdr	*ip,*vxip;
	struct myip6hdr	*ip6;
	struct myudphdr		*udp;
	struct myvxlanhdr		*vxlan;
	struct myetherhdr		*etherpacket;

	if (opt_debug == TRUE) {
		printf("[send_ip] This is the data received!\n");
		unsigned int i;
		for (i=0; i<datalen; i++)
			printf("%.2X ", data[i]&255);
			printf("\n");
	}

	// Allocate space for a packet, or IPv6 ethernet header + packet
	if (!opt_inet6mode ) {
		packetsize = IPHDR_SIZE + optlen + datalen;
		if ( (packet = malloc(packetsize)) == NULL) {
			perror("[send_ip] inet malloc()");
			return;
		}
		memset(packet, 0, packetsize);
	}
	else {
		// IPv6 adds an Etherframe in front of packet
		packetsize = IP6HDR_SIZE + optlen + datalen;
		if ( (ether_frame = malloc(packetsize + ETHERHDR_SIZE)) == NULL) {
			perror("[send_ip] inet6 malloc() failed");
			return;
		}
		memset(ether_frame, 0, packetsize + ETHERHDR_SIZE);
		packet = (ether_frame + ETHERHDR_SIZE);
	}

	//printf("[sendip] Malloc OK\n");

	// Lets make an IPv4 packet
	if (! opt_inet6mode) {

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

	printf("Debug packet\n");
	if (opt_debug == TRUE)
		{
				unsigned int i;
				printf("This is the packet to be sent!\n");
				for (i=0; i<packetsize; i++)
						printf("%.2X ", packet[i]&255);
				printf("\n");
		}
	} else
	// Lets make an IPv6 packet!
	{
		//printf("[sendip] Making an IPv6 packet\n");
		/* IPv6 header */
		ip6 = (struct myip6hdr *) packet;
		ip6->version	= 6;

		/* IPv6 version (4 bits), Traffic class (8 bits), Flow label (20 bits) */
		/*ip6->ip6_flow = htonl ((6 << 28) | (0 << 20) | 0);*/

		/* Next header (8 bits): 44 for Frag */
		//ip6->next_hdr = 44;

		/* Hop limit (8 bits): default to maximum value */
		ip6->hop_limit = 64;
		ip6->payload_len	= htons(packetsize);
		/* copy src and dst address */
		memcpy(&ip6->saddr, src, sizeof(ip6->saddr));
		memcpy(&ip6->daddr, dst, sizeof(ip6->daddr));

		//printf("[sendip] Making an IPv6 packet - done \n");
	}

	/* Make it into a VXLAN packet, add IP/UDP */
	if (vxlanmode != 0) {
		printf("[sendip] vxlanmode set\n");
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
		memcpy(&vxip->saddr, (struct sockaddr*)&vxlan_local.sin_addr.s_addr, sizeof(vxip->saddr));
		memcpy(&vxip->daddr, (struct sockaddr*)&vxlan_remote.sin_addr.s_addr, sizeof(vxip->daddr));
		//(struct sockaddr*)&vxlan_local.sin_addr.s_addr

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
		etherpacket =  (struct myetherhdr*) (vxpacket+IPHDR_SIZE+UDPHDR_SIZE+VXLANHDR_SIZE);
		memcpy(&etherpacket->dest, vxdst_mac, 6);
		memcpy(&etherpacket->source, vxsrc_mac, 6);
		etherpacket->type	= htons(0x0800);

		memcpy(packet + IPHDR_SIZE + optlen, data, datalen);
		memcpy(vxpacket + IPHDR_SIZE + UDPHDR_SIZE + VXLANHDR_SIZE + ETHERHDR_SIZE , packet, packetsize);

		if (opt_debug == TRUE) {
			unsigned int i;
			printf("VXLAN packet debug\n");
			for (i=0; i<vxpacketsize; i++)
				printf("%.2X ", vxpacket[i]&255);
			printf("\n");
		}
		free(packet);
		packet = vxpacket;
	}
	else if ( ! opt_inet6mode ){
		/* copies data */
		printf("[sendip] Segmentation fault here?\n");
		memcpy(packet + IPHDR_SIZE + optlen, data, datalen);
	}



	// Sending here?
	if (! opt_inet6mode) {
		result = sendto(sockraw, packet, packetsize, 0,
			(struct sockaddr*)&remote, sizeof(remote));
		free(packet);
	} else {
		//printf("[sendip] Preparing Ethernet frame with IPv6 packet\n");
		// To control this part - need more than raw, we use Ethernet frames
		// Todo: incomplete, should find MAC addresses and enter here!
		/* Destination and Source MAC addresses */
		//memcpy(ether_frame, dst_mac, 6 * sizeof (uint8_t));
		//memcpy(ether_frame + 6, src_mac, 6 * sizeof (uint8_t));
		/* Next is ethernet type code (ETH_P_IPV6 for IPv6) */


		// IPv6 needs to be sent as an ethernet frame
		// Only IPv6 right now, type 0x0800, IPv6 would bx 0x86DD
		etherpacket =  (struct myetherhdr*) ether_frame;
		memcpy(&etherpacket->dest, dst_mac, 6);
		memcpy(&etherpacket->source, src_mac, 6);
		etherpacket->type	= htons(0x86DD);

		if (opt_rawipmode)	ip6->next_hdr = raw_ip_protocol;
		else if	(opt_icmpmode)	ip6->next_hdr = 1;	/* icmp */
		else if (opt_udpmode)	ip6->next_hdr = 17;	/* udp  */
		else			ip6->next_hdr = 6;	/* tcp  */

		// Copy rest of packet
		memcpy(packet + IP6HDR_SIZE + optlen, data, datalen);

		if (opt_debug == TRUE) {
			printf("[sendip] This is the Ethernet frame to be sent!\n");
			unsigned int i;
			for (i=0; i<packetsize + ETHERHDR_SIZE; i++)
				printf("%.2X ", ether_frame[i]&255);
				printf("\n");
		}
		result = sendto(sockraw, ether_frame, packetsize + ETHERHDR_SIZE , 0,
				(struct sockaddr *) &rawdevice, sizeof (rawdevice));
		free(ether_frame);
	}

	if (result == -1 && errno != EINTR && !opt_rand_dest && !opt_rand_source) {
		perror("[send_ip] sendto");
		if (close(sockraw) == -1)
			perror("[ipsender] close(sockraw)");
		if (close_pcap() == -1)
			printf("[ipsender] close_pcap failed\n");
		exit(1);
	}

	/* inc packet id for safe protocol */
	if (opt_safe && !eof_reached)
		src_id++;
}
