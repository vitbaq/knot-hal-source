#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "config.h"
#include "abstract_driver.h"
#include "src/nrf24l01/nrf24l01.h"

//
//  How to access GPIO registers from C-code on the Raspberry-Pi
//  Example program
//  15-January-2012
//  Dom and Gert
//  Revised: 15-Feb-2013


// Access from ARM Running Linux

#define BCM2708_PERI_BASE	0x3F000000
#define GPIO_BASE                		(BCM2708_PERI_BASE + 0x200000) /* GPIO controller */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <time.h>

#define PAGE_SIZE (4*1024)
#define BLOCK_SIZE (4*1024)

#define CE 25

#if 0
int  mem_fd;
void *gpio_map;

// I/O access
volatile unsigned *gpio;

// GPIO setup macros. Always use INP_GPIO(x) before using OUT_GPIO(x) or SET_GPIO_ALT(x,y)
#define INP_GPIO(g) *(gpio+((g)/10)) &= ~(7<<(((g)%10)*3))
#define OUT_GPIO(g) *(gpio+((g)/10)) |=  (1<<(((g)%10)*3))
#define SET_GPIO_ALT(g,a) *(gpio+(((g)/10))) |= (((a)<=3?(a)+4:(a)==4?3:2)<<(((g)%10)*3))

#define GPIO_SET *(gpio+7)  // sets   bits which are 1 ignores bits which are 0
#define GPIO_CLR *(gpio+10) // clears bits which are 1 ignores bits which are 0

#define GET_GPIO(g) (*(gpio+13)&(1<<g)) // 0 if LOW, (1<<g) if HIGH

#define GPIO_PULL *(gpio+37) // Pull up/pull down
#define GPIO_PULLCLK0 *(gpio+38) // Pull up/pull down clock

static void setup_io()
{
	/* open /dev/mem */
	if ((mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0) {
		printf("can't open /dev/mem \n");
		exit(-1);
	}

	/* mmap GPIO */
	gpio_map = mmap(
										NULL,             //Any adddress in our space will do
										BLOCK_SIZE,       //Map length
										PROT_READ|PROT_WRITE,// Enable reading & writting to mapped memory
										MAP_SHARED,       //Shared with other processes
										mem_fd,           //File to map
										GPIO_BASE         //Offset to GPIO peripheral
										);

	close(mem_fd); //No need to keep mem_fd open after mmap

	if (gpio_map == MAP_FAILED) {
		printf("mmap error %d\n", (int)gpio_map);//errno also set!
		exit(-1);
	}

	// Always use volatile pointer!
	gpio = (volatile unsigned *)gpio_map;
} // setup_io

#else

#define GPIO_BASE 		(BCM2708_PERI_BASE + 0x200000)
#define PAGE_SIZE		(4*1024)
#define BLOCK_SIZE		(4*1024)

//Raspberry pi GPIO Macros
#define INP_GPIO(g)					*(gpio+((g)/10)) &= ~(7<<(((g)%10)*3))
#define OUT_GPIO(g)				*(gpio+((g)/10)) |=  (1<<(((g)%10)*3))
#define SET_GPIO_ALT(g,a)	*(gpio+(((g)/10))) |= (((a)<=3?(a)+4:(a)==4?3:2)<<(((g)%10)*3))

#define GPIO_SET						*(gpio+7)  // sets   bits which are 1 ignores bits which are 0
#define GPIO_CLR						*(gpio+10) // clears bits which are 1 ignores bits which are 0

#define GET_GPIO(g)				(*(gpio+13)&(1<<g)) // 0 if LOW, (1<<g) if HIGH

#define GPIO_PULL					*(gpio+37) // Pull up/pull down
#define GPIO_PULLCLK0			*(gpio+38) // Pull up/pull down clock

static volatile unsigned				*gpio;

//
// Set up a memory regions to access GPIO
//
static void setup_io()
{
	//open /dev/mem
	int mem_fd = open("/dev/mem", O_RDWR|O_SYNC);
	if (mem_fd < 0) {
		printf("can't open /dev/mem \n");
		exit(-1);
	}

	gpio = (volatile unsigned*)mmap(NULL,
																BLOCK_SIZE,
																PROT_READ | PROT_WRITE,
																MAP_SHARED,
																mem_fd,
																GPIO_BASE);
   	close(mem_fd);
   	if (gpio == MAP_FAILED) {
      		printf("mmap error\n");
      		exit(-1);
   	}

	GPIO_CLR = (1<<CE);
	INP_GPIO(CE);
	OUT_GPIO(CE);

	GPIO_CLR = (1<<CE);
	//disable();
	//spi_init();
}
#endif

void printButton(int g)
{
  if (GET_GPIO(g)) // !=0 <-> bit is 1 <- port is HIGH=3.3V
    printf("Button pressed!\n");
  else // port is LOW=0V
    printf("Button released!\n");
}

int main(int argc, char **argv)
{
	int g,rep, timeout;

	int fdsrv, fdcli;

//	puts ("Hello C World!");
//	printf("This is " PACKAGE_STRING "\n");
//	printf("Driver name=%s\n" , nrf24l01_driver.name);
//
//	/* Intializes random number generator */
//	srand((unsigned)time(NULL));
//	timeout = ((rand() % 50) + 1)  * 20;
//	srand((unsigned)time(NULL));
//	printf("addr=%#04x - %#08x timeout=%d\n", (rand() ^ rand()) & 0xffff, 	rand() ^ (rand() * 65536), timeout);
//
//	printf("sizeof(len_t)=%ld max=%lu\n", sizeof(len_t), LEN_T_MAX);
//	printf("sizeof(int_t)=%ld\n", sizeof(int_t));
//	printf("sizeof(result_t)=%ld\n", sizeof(result_t));
//	printf("sizeof(param_t)=%ld\n", sizeof(int_t));
//	printf("sizeof(long unsigned int)=%ld\n", sizeof(long unsigned int));
//	printf("sizeof(int)=%ld\n", sizeof(int));

	// Set up gpi pointer for direct register access
//	setup_io();

	// Switch GPIO 7..11 to output mode

	/************************************************************************\
	* You are about to change the GPIO settings of your computer.          *
	* Mess this up and it will stop working!                               *
	* It might be a good idea to 'sync' before running this program        *
	* so at least you still have your code changes written to the SD-card! *
	\************************************************************************/

	// Set GPIO pins 7-11 to output
//	for (g=7; g<=11; g++)
//	{
		//INP_GPIO(25); // must use INP_GPIO before we can use OUT_GPIO
		//OUT_GPIO(25);
//	}

//	printf("gpio %p\n", gpio);
//
//	for (rep=0; rep<10; rep++)
//	{
////	for (g=7; g<=11; g++)
////	{
//		GPIO_SET = 1<<CE;
//		sleep(1);
////	}
////	for (g=7; g<=11; g++)
////	{
//		GPIO_CLR = 1<<CE;
//		sleep(1);
////	}
//	}
//
//	nrf24l01_driver.probe();
//	printf("CE on\n");
//	nrf24l01_ce_on();
//	nrf24l01_driver.remove();

	nrf24l01_driver.probe();

	fdsrv = nrf24l01_driver.socket();
	if (fdsrv == ERROR) {
		printf("socket(%d): %s", errno, strerror(errno));
		nrf24l01_driver.remove();
		return 1;
	}

	if (nrf24l01_driver.listen(fdsrv, 10) == ERROR) {
		printf("listen(%d): %s", errno, strerror(errno));
		nrf24l01_driver.close(fdsrv);
		nrf24l01_driver.remove();
		return 2;
	}

	fdcli = nrf24l01_driver.accept(fdsrv);
	if (fdcli == ERROR) {
		printf("accept(%d): %s", errno, strerror(errno));
	} else {
		nrf24l01_driver.close(fdcli);
	}

	nrf24l01_driver.close(fdsrv);
	nrf24l01_driver.remove();
	return 0;
} // main
