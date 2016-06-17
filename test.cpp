#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#include "abstract_driver.h"

// application packet size maximum
#define PACKET_SIZE_MAX		86

#define MESSAGE				"the quick brown fox jumps over the lazy dog"
#define MESSAGE_SIZE	(sizeof(MESSAGE)-1)

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
		  size,
		  count,
		  inc;
	bool connected = false;
	ssize_t nbytes;
	char	msg[PACKET_SIZE_MAX],
				buffer[PACKET_SIZE_MAX];

	signal(SIGTERM, sig_term);
	signal(SIGINT, sig_term);

	fprintf(stdout, "Client started...\n");

	if (nrf24l01_driver.probe(PACKET_SIZE_MAX) == ERROR) {
		fprintf(stderr, " probe(%d): %s\nClient finished.\n", errno, strerror(errno));
		return 1;
	}

	fprintf(stdout, "NRF24L01 ok\n");

	for (size=0, inc=MESSAGE_SIZE; size<sizeof(msg);) {
		if ((inc+size) > sizeof(msg)) {
			inc = sizeof(msg) - size;
		}
		size += sprintf(msg+size, "%.*s", inc, MESSAGE);
	}

	while(!m_bbreak) {
		if (!connected) {
			sock = nrf24l01_driver.socket();
			if (sock == ERROR) {
				fprintf(stderr, " socket(%d): %s\n", errno, strerror(errno));
				nrf24l01_driver.remove();
			}
			fprintf(stdout, "Client connecting...\n");
			if (nrf24l01_driver.connect(sock, &channel, sizeof(channel)) == ERROR) {
				fprintf(stderr, " connect(%d): %s\n", errno, strerror(errno));
				nrf24l01_driver.close(sock);
			} else {
				fprintf(stdout, " connect(%d): %s\n", errno, strerror(errno));
				connected = true;
				count = 1;
			}
		} else {
			fprintf(stdout, "CLIENT:\n");
			nbytes = sprintf(buffer, "%.*s", count, msg);
			inc = (count == size) ? -1 : (count == 1 ? 1 : inc);
			count += inc;
			if (nrf24l01_driver.write(sock, buffer, nbytes) < 0) {
				fprintf(stderr, " write(%d): %s\n", errno, strerror(errno));
				nrf24l01_driver.close(sock);
				connected = false;
			} else {
				fprintf(stdout, "TX(%ld): '%.*s'\n", nbytes, (int)nbytes, buffer);
			}
			while(connected) {
				nbytes = nrf24l01_driver.read(sock, buffer, sizeof(buffer));
				if (nbytes < 0) {
					fprintf(stderr, " read(%d): %s\n", errno, strerror(errno));
					nrf24l01_driver.close(sock);
					connected = false;
				} else 	if (nbytes == 0) {
					usleep(10);
				} else {
					fprintf(stdout, "RX(%ld): '%.*s'\n", nbytes, (int)nbytes, buffer);
					break;
				}
			}
		}
	}

	nrf24l01_driver.remove();
	fprintf(stdout, "Client finished.\n");

	return 0;
}
