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
	int channel = 10,
		  count;
	bool connected = false;
	ssize_t nbytes;
	char buffer[PACKET_SIZE_MAX];

	signal(SIGTERM, sig_term);
	signal(SIGINT, sig_term);

	fprintf(stdout, "Client started...\n");

	nrf24l01_driver.probe(PACKET_SIZE_MAX);

	fprintf(stdout, "NRF24L01 uploaded OK\n");

	while(!m_bbreak) {
		if (!connected) {
			sock = nrf24l01_driver.socket();
			if (sock == ERROR) {
				fprintf(stderr, "socket(%d): %s\n", errno, strerror(errno));
				nrf24l01_driver.remove();
			}
			fprintf(stdout, "Client connecting...\n");
			if (nrf24l01_driver.connect(sock, &channel, sizeof(channel)) == ERROR) {
				fprintf(stderr, " connect(%d): %s\n", errno, strerror(errno));
				nrf24l01_driver.close(sock);
			} else {
				fprintf(stdout, " connect(%d): %s\n", errno, strerror(errno));
				connected = true;
				count = 0;
			}
		} else {
			nbytes = nrf24l01_driver.read(sock, buffer, sizeof(buffer));
			if (nbytes < 0) {
				nrf24l01_driver.close(sock);
				connected = false;
			}
			if (nbytes > 0) {
				fprintf(stdout, " MSG(%ld)>> '%.*s'\n", nbytes, (int)nbytes, buffer);
			}
			nbytes = sprintf(buffer, "Diogenes %d", ++count);
			nbytes = nrf24l01_driver.write(sock, buffer, nbytes);
			fprintf(stdout, "write(%d): %s\n", errno, strerror(errno));
		}
		sleep(1);
	}

	nrf24l01_driver.remove();
	fprintf(stdout, "Client finished.\n");

	return 0;
}
