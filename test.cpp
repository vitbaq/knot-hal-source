#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#include "abstract_driver.h"
#include "util.h"

// application packet size maximum
#define PACKET_SIZE_MAX		128

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
		  msg_count,
		  inc;
	bool connected = false;
	ssize_t nbytes;
	char	msg[PACKET_SIZE_MAX],
				buffer[PACKET_SIZE_MAX];

	signal(SIGTERM, sig_term);
	signal(SIGINT, sig_term);

	fprintf(stdout, "Client started...\n");

	if (nrf24l01_driver.probe(PACKET_SIZE_MAX) < 0) {
		fprintf(stderr, " probe(%d): %s\nClient finished.\n", errno, strerror(errno));
		return 1;
	}

	fprintf(stdout, "NRF24L01 loaded\n");

	for (size=0, inc=MESSAGE_SIZE; size<sizeof(msg);) {
		if ((inc+size) > sizeof(msg)) {
			inc = sizeof(msg) - size;
		}
		size += sprintf(msg+size, "%.*s", inc, MESSAGE);
	}

	while(!m_bbreak) {
		if (!connected) {
			sock = nrf24l01_driver.socket();
			if (sock < 0) {
				fprintf(stderr, " socket(%d): %s\n", errno, strerror(errno));
				return 2;
			}

			fprintf(stdout, "Client connecting...\n");
			if (nrf24l01_driver.connect(sock, &channel, sizeof(channel)) < 0) {
				fprintf(stderr, " connect(%d): %s\n", errno, strerror(errno));
				nrf24l01_driver.close(sock);
				continue;
			}

			fprintf(stdout, " connect(%d): %s\n", errno, strerror(errno));
			connected = true;
			count = get_random_value(size, 1, 1);
			inc = 1;
			msg_count = 0;
		}
		nbytes = sprintf(buffer, "%.*s", count, msg);
		inc = (count == size) ? -1 : (count == 1 ? 1 : inc);
		count += inc;
		if (nrf24l01_driver.write(sock, buffer, nbytes) < 0) {
			fprintf(stderr, " write(%d): %s\n", errno, strerror(errno));
			nrf24l01_driver.close(sock);
			connected = false;
		} else {
			fprintf(stdout, "TX:[%03ld]: %d-'%.*s'\n", nbytes, ++msg_count, (int)nbytes, buffer);
		}
		while(connected) {
			nbytes = nrf24l01_driver.read(sock, buffer, sizeof(buffer));
			if (nbytes < 0) {
				fprintf(stderr, " read(%d): %s\n", errno, strerror(errno));
				nrf24l01_driver.close(sock);
				connected = false;
			} else if (nbytes == 0) {
				usleep(10);
			} else {
				fprintf(stdout, "RX:[%03ld]: '%.*s'\n", nbytes, (int)nbytes, buffer);
				break;
			}
		}
	}

	nrf24l01_driver.remove();
	fprintf(stdout, "Client finished.\n");

	return 0;
}
