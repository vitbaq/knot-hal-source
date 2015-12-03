#include <iostream>

#include "config.h"
#include "src/abstract_driver.h"

using namespace std;

int main(void)
{
  cout << "Hello C++ World, " << PACKAGE_STRING << std::endl;
  cout << "Driver name=" << driver.name << std::endl;
  return 0;
}
