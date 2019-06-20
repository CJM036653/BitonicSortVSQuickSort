#include <iostream>
#include <stdlib.h>
#include <time.h>

const int MAX_VALUE = 1000;
const int AR_SIZE = 2097152;

int main()
{
  srand(time(NULL));
  int arSize;
  std::cin >> arSize;

  for (int i = 0; i < arSize; i++)
  {
    std::cout << rand() % MAX_VALUE << std::endl;
  }
  return 0;
}
