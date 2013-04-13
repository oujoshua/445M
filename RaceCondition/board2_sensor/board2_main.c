#include "OS.h"
#include "shell.h"


int main(void) {
  OS_Init();
  
  OS_Launch(TIME_2MS);
  return 0;
}
