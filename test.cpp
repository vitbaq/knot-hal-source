#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#include "abstract_driver.h"

// application packet size maximum
#define PACKET_SIZE_MAX		32

static bool	 m_bbreak = false;

static void sig_term(int sig)
{
	m_bbreak = true;
	putchar('\n');
}

int main(void)
{
	int sock;
	int channel = 10;
	ssize_t nbytes;
	char buffer[PACKET_SIZE_MAX];

	signal(SIGTERM, sig_term);
	signal(SIGINT, sig_term);

	fprintf(stdout, "Client started...\n");

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

	fprintf(stdout, "Connect(%d): %s\n", errno, strerror(errno));
	while(!m_bbreak) {
		nbytes = nrf24l01_driver.read(sock, buffer, sizeof(buffer));
		printf("nbytes=%ld\n", nbytes);
		sleep(1);
	}

	nrf24l01_driver.remove();
	fprintf(stdout, "Finished\n");
	return 0;
}
