#include <stdio.h>

#include "config.h"
#include "abstract_driver.h"

int main (void)
{
  puts ("Hello C World!");

  printf("This is " PACKAGE_STRING "\n");
  printf("Driver name=%s\n" , nrf24l01_driver.name);

  return 0;
}
