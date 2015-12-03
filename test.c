#include <stdio.h>
#include "config.h"
#include "src/abstract_driver.h"

int main (void)
{
  puts ("Hello C World!");

  printf("This is " PACKAGE_STRING "\n");
  printf("Driver name=%s\n" , driver.name);

  return 0;
}
