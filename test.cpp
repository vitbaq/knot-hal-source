#include <iostream>
#include <stdio.h>

#include "config.h"
#include "abstract_driver.h"
#include "src/nrf24l01/nrf24l01.h"

using namespace std;

int main(void)
{
	int count = 0;
	union {
	 uint64_t b64;
	 long unsigned int b32[2];
	} v;

	cout << "Hello C++ World! Testcpp make buildroot"  << std::endl;
	cout << "This is " << PACKAGE_STRING << std::endl;
	cout << "Driver name=" << driver.name << std::endl;

	v.b64 = 0;

	nrf24l01_init();

	printf("[%d]RX: register=0x%02x status=%#02x\n", ++count, nrf24l01_inr(CONFIG), nrf24l01_comand(NOP));
	printf("RX: EN_RXADDR=0x%02x EN_AA=%#02x\n", nrf24l01_inr(EN_RXADDR), nrf24l01_inr(EN_RXADDR));
	nrf24l01_open_pipe(0);
	printf("RX: PIPE0 EN_RXADDR=0x%02x EN_AA=%#02x\n", nrf24l01_inr(EN_RXADDR), nrf24l01_inr(EN_AA));
	nrf24l01_open_pipe(1);
	printf("RX: PIPE1 EN_RXADDR=0x%02x EN_AA=%#02x\n", nrf24l01_inr(EN_RXADDR), nrf24l01_inr(EN_AA));
	nrf24l01_open_pipe(2);
	printf("RX: PIPE2 EN_RXADDR=0x%02x EN_AA=%#02x\n", nrf24l01_inr(EN_RXADDR), nrf24l01_inr(EN_AA));
	nrf24l01_open_pipe(3);
	printf("RX: PIPE3 EN_RXADDR=0x%02x EN_AA=%#02x\n", nrf24l01_inr(EN_RXADDR), nrf24l01_inr(EN_AA));
	nrf24l01_open_pipe(4);
	printf("RX: PIPE4 EN_RXADDR=0x%02x EN_AA=%#02x\n", nrf24l01_inr(EN_RXADDR), nrf24l01_inr(EN_AA));
	nrf24l01_open_pipe(5);
	printf("RX: PIPE5 EN_RXADDR=0x%02x EN_AA=%#02x\n", nrf24l01_inr(EN_RXADDR), nrf24l01_inr(EN_AA));
	nrf24l01_inr_data(RX_ADDR_P0, &v.b64, AW_RD(nrf24l01_inr(SETUP_AW)));
	printf("    RX_ADDR_P0=0x%lx%lx\r\n", v.b32[1], v.b32[0]);
	nrf24l01_inr_data(RX_ADDR_P1, &v.b64, AW_RD(nrf24l01_inr(SETUP_AW)));
	printf("    RX_ADDR_P1=0x%lx%lx\r\n", v.b32[1], v.b32[0]);
	nrf24l01_inr_data(RX_ADDR_P2, &v.b64, 1);
	printf("    RX_ADDR_P2=0x%lx%lx\r\n", v.b32[1], v.b32[0]);
	nrf24l01_inr_data(RX_ADDR_P3, &v.b64, 1);
	printf("    RX_ADDR_P3=0x%lx%lx\r\n", v.b32[1], v.b32[0]);
	nrf24l01_inr_data(RX_ADDR_P4, &v.b64, 1);
	printf("    RX_ADDR_P4=0x%lx%lx\r\n", v.b32[1], v.b32[0]);
	nrf24l01_inr_data(RX_ADDR_P5, &v.b64, 1);
	printf("    RX_ADDR_P5=0x%lx%lx\r\n", v.b32[1], v.b32[0]);
	nrf24l01_set_address_pipe(TX_ADDR, 0);
	nrf24l01_inr_data(TX_ADDR, &v.b64, AW_RD(nrf24l01_inr(SETUP_AW)));
	printf("    TX_ADDR=0x%lx%lx\r\n", v.b32[1], v.b32[0]);
	nrf24l01_set_address_pipe(TX_ADDR, 5);
	nrf24l01_inr_data(TX_ADDR, &v.b64, AW_RD(nrf24l01_inr(SETUP_AW)));
	printf("    TX_ADDR=0x%lx%lx\r\n", v.b32[1], v.b32[0]);
	nrf24l01_set_address_pipe(TX_ADDR, 4);
	nrf24l01_inr_data(TX_ADDR, &v.b64, AW_RD(nrf24l01_inr(SETUP_AW)));
	printf("    TX_ADDR=0x%lx%lx\r\n", v.b32[1], v.b32[0]);
	nrf24l01_set_address_pipe(TX_ADDR, 3);
	nrf24l01_inr_data(TX_ADDR, &v.b64, AW_RD(nrf24l01_inr(SETUP_AW)));
	printf("    TX_ADDR=0x%lx%lx\r\n", v.b32[1], v.b32[0]);
	nrf24l01_set_address_pipe(TX_ADDR, 2);
	nrf24l01_inr_data(TX_ADDR, &v.b64, AW_RD(nrf24l01_inr(SETUP_AW)));
	printf("    TX_ADDR=0x%lx%lx\r\n", v.b32[1], v.b32[0]);
	nrf24l01_set_address_pipe(TX_ADDR, 1);
	nrf24l01_inr_data(TX_ADDR, &v.b64, AW_RD(nrf24l01_inr(SETUP_AW)));
	printf("    TX_ADDR=0x%lx%lx\r\n", v.b32[1], v.b32[0]);
	nrf24l01_close_pipe(5);
	printf("RX: PIPE5 EN_RXADDR=0x%02x EN_AA=%#02x\n", nrf24l01_inr(EN_RXADDR), nrf24l01_inr(EN_AA));
	nrf24l01_close_pipe(4);
	printf("RX: PIPE4 EN_RXADDR=0x%02x EN_AA=%#02x\n", nrf24l01_inr(EN_RXADDR), nrf24l01_inr(EN_AA));
	nrf24l01_close_pipe(3);
	printf("RX: PIPE3 EN_RXADDR=0x%02x EN_AA=%#02x\n", nrf24l01_inr(EN_RXADDR), nrf24l01_inr(EN_AA));
	nrf24l01_close_pipe(2);
	printf("RX: PIPE2 EN_RXADDR=0x%02x EN_AA=%#02x\n", nrf24l01_inr(EN_RXADDR), nrf24l01_inr(EN_AA));
	nrf24l01_close_pipe(1);
	printf("RX: PIPE1 EN_RXADDR=0x%02x EN_AA=%#02x\n", nrf24l01_inr(EN_RXADDR), nrf24l01_inr(EN_AA));
	nrf24l01_close_pipe(0);
	printf("RX: PIPE0 EN_RXADDR=0x%02x EN_AA=%#02x\n", nrf24l01_inr(EN_RXADDR), nrf24l01_inr(EN_AA));

	nrf24l01_deinit();
	nrf24l01_init();
	printf("[%d]RX: register=0x%02x status=%#02x\n", ++count, nrf24l01_inr(CONFIG), nrf24l01_comand(NOP));
	printf("RX: EN_RXADDR=0x%02x EN_AA=%#02x\n", nrf24l01_inr(EN_RXADDR), nrf24l01_inr(EN_RXADDR));
	nrf24l01_open_pipe(0);
	printf("RX: PIPE0 EN_RXADDR=0x%02x EN_AA=%#02x\n", nrf24l01_inr(EN_RXADDR), nrf24l01_inr(EN_AA));
	nrf24l01_open_pipe(1);
	printf("RX: PIPE1 EN_RXADDR=0x%02x EN_AA=%#02x\n", nrf24l01_inr(EN_RXADDR), nrf24l01_inr(EN_AA));
	nrf24l01_open_pipe(2);
	printf("RX: PIPE2 EN_RXADDR=0x%02x EN_AA=%#02x\n", nrf24l01_inr(EN_RXADDR), nrf24l01_inr(EN_AA));
	nrf24l01_open_pipe(3);
	printf("RX: PIPE3 EN_RXADDR=0x%02x EN_AA=%#02x\n", nrf24l01_inr(EN_RXADDR), nrf24l01_inr(EN_AA));
	nrf24l01_open_pipe(4);
	printf("RX: PIPE4 EN_RXADDR=0x%02x EN_AA=%#02x\n", nrf24l01_inr(EN_RXADDR), nrf24l01_inr(EN_AA));
	nrf24l01_open_pipe(5);
	printf("RX: PIPE5 EN_RXADDR=0x%02x EN_AA=%#02x\n", nrf24l01_inr(EN_RXADDR), nrf24l01_inr(EN_AA));
	nrf24l01_inr_data(RX_ADDR_P0, &v.b64, AW_RD(nrf24l01_inr(SETUP_AW)));
	printf("    RX_ADDR_P0=0x%lx%lx\r\n", v.b32[1], v.b32[0]);
	nrf24l01_inr_data(RX_ADDR_P1, &v.b64, AW_RD(nrf24l01_inr(SETUP_AW)));
	printf("    RX_ADDR_P1=0x%lx%lx\r\n", v.b32[1], v.b32[0]);
	nrf24l01_inr_data(RX_ADDR_P2, &v.b64, 1);
	printf("    RX_ADDR_P2=0x%lx%lx\r\n", v.b32[1], v.b32[0]);
	nrf24l01_inr_data(RX_ADDR_P3, &v.b64, 1);
	printf("    RX_ADDR_P3=0x%lx%lx\r\n", v.b32[1], v.b32[0]);
	nrf24l01_inr_data(RX_ADDR_P4, &v.b64, 1);
	printf("    RX_ADDR_P4=0x%lx%lx\r\n", v.b32[1], v.b32[0]);
	nrf24l01_inr_data(RX_ADDR_P5, &v.b64, 1);
	printf("    RX_ADDR_P5=0x%lx%lx\r\n", v.b32[1], v.b32[0]);
	nrf24l01_set_address_pipe(TX_ADDR, 0);
	nrf24l01_inr_data(TX_ADDR, &v.b64, AW_RD(nrf24l01_inr(SETUP_AW)));
	printf("    TX_ADDR=0x%lx%lx\r\n", v.b32[1], v.b32[0]);
	nrf24l01_set_address_pipe(TX_ADDR, 5);
	nrf24l01_inr_data(TX_ADDR, &v.b64, AW_RD(nrf24l01_inr(SETUP_AW)));
	printf("    TX_ADDR=0x%lx%lx\r\n", v.b32[1], v.b32[0]);
	nrf24l01_set_address_pipe(TX_ADDR, 4);
	nrf24l01_inr_data(TX_ADDR, &v.b64, AW_RD(nrf24l01_inr(SETUP_AW)));
	printf("    TX_ADDR=0x%lx%lx\r\n", v.b32[1], v.b32[0]);
	nrf24l01_set_address_pipe(TX_ADDR, 3);
	nrf24l01_inr_data(TX_ADDR, &v.b64, AW_RD(nrf24l01_inr(SETUP_AW)));
	printf("    TX_ADDR=0x%lx%lx\r\n", v.b32[1], v.b32[0]);
	nrf24l01_set_address_pipe(TX_ADDR, 2);
	nrf24l01_inr_data(TX_ADDR, &v.b64, AW_RD(nrf24l01_inr(SETUP_AW)));
	printf("    TX_ADDR=0x%lx%lx\r\n", v.b32[1], v.b32[0]);
	nrf24l01_set_address_pipe(TX_ADDR, 1);
	nrf24l01_inr_data(TX_ADDR, &v.b64, AW_RD(nrf24l01_inr(SETUP_AW)));
	printf("    TX_ADDR=0x%lx%lx\r\n", v.b32[1], v.b32[0]);
	nrf24l01_close_pipe(5);
	printf("RX: PIPE5 EN_RXADDR=0x%02x EN_AA=%#02x\n", nrf24l01_inr(EN_RXADDR), nrf24l01_inr(EN_AA));
	nrf24l01_close_pipe(4);
	printf("RX: PIPE4 EN_RXADDR=0x%02x EN_AA=%#02x\n", nrf24l01_inr(EN_RXADDR), nrf24l01_inr(EN_AA));
	nrf24l01_close_pipe(3);
	printf("RX: PIPE3 EN_RXADDR=0x%02x EN_AA=%#02x\n", nrf24l01_inr(EN_RXADDR), nrf24l01_inr(EN_AA));
	nrf24l01_close_pipe(2);
	printf("RX: PIPE2 EN_RXADDR=0x%02x EN_AA=%#02x\n", nrf24l01_inr(EN_RXADDR), nrf24l01_inr(EN_AA));
	nrf24l01_close_pipe(1);
	printf("RX: PIPE1 EN_RXADDR=0x%02x EN_AA=%#02x\n", nrf24l01_inr(EN_RXADDR), nrf24l01_inr(EN_AA));
	nrf24l01_close_pipe(0);
	printf("RX: PIPE0 EN_RXADDR=0x%02x EN_AA=%#02x\n", nrf24l01_inr(EN_RXADDR), nrf24l01_inr(EN_AA));

	return 0;
}
