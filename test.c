#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#include "config.h"
#include "abstract_driver.h"

int m_fdsrv = SOCKET_INVALID;

static void sig_term(int sig)
{
	nrf24l01_driver.cancel(m_fdsrv);
}

int main(int argc, char **argv)
{
	int fdcli;

	printf("TEST KNOT HAL SERVER\n");

	signal(SIGTERM, sig_term);
	signal(SIGINT, sig_term);

	nrf24l01_driver.probe();

	m_fdsrv = nrf24l01_driver.socket();
	if (m_fdsrv == ERROR) {
		printf("socket(%d): %s\n", errno, strerror(errno));
		nrf24l01_driver.remove();
		return 1;
	}

	if (nrf24l01_driver.listen(m_fdsrv, 10) == ERROR) {
		printf("listen(%d): %s\n", errno, strerror(errno));
		nrf24l01_driver.close(m_fdsrv);
		nrf24l01_driver.remove();
		return 2;
	}

	fdcli = nrf24l01_driver.accept(m_fdsrv);
	if (fdcli == ERROR) {
		printf("accept(%d): %s\n", errno, strerror(errno));
	} else {
		nrf24l01_driver.close(fdcli);
	}

	nrf24l01_driver.close(m_fdsrv);
	nrf24l01_driver.remove();
	return 0;
}
