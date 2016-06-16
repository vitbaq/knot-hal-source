#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "abstract_driver.h"

// application packet size maximum
#define PACKET_SIZE_MAX		32

int main(void)
{
	int sock;
	int channel = 10;

	nrf24l01_driver.probe(PACKET_SIZE_MAX);

	sock = nrf24l01_driver.socket();
	if (sock == ERROR) {
		fprintf(stderr, "socket(%d): %s\n", errno, strerror(errno));
		nrf24l01_driver.remove();
		return 1;
	}

	if (nrf24l01_driver.connect(sock, &channel, sizeof(channel)) == ERROR) {
		fprintf(stderr, "listen(%d): %s\n", errno, strerror(errno));
		nrf24l01_driver.close(sock);
		nrf24l01_driver.remove();
		return 2;
	}

	nrf24l01_driver.remove();
	return 0;
}
