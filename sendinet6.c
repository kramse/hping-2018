/* $Id: sendrawip.c,v 1.2 2003/09/01 00:22:06 antirez Exp $ */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#include "hping2.h"
#include "globals.h"

void send_inet6(void)
{
	char *packet;

	perror("[send_inet6] Getting ready!\n");
	packet = malloc(data_size);
	if (packet == NULL) {
		perror("[send_inet6] malloc()");
		return;
	}
	perror("[send_inet6] Checkpoint\n");
	memset(packet, 0, data_size);
	perror("[send_inet6] set!");
	data_handler(packet, data_size);
	perror("[send_inet6] send_ip!");
	send_ip_handler(packet, data_size);
	free(packet);
}
