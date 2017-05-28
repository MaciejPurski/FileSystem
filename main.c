#include <time.h>
#include <stdio.h>
#include "filesystem.h"

int main() {
  create_disc(20000, "Mydisk");
  show_fat();
  
  
  return 0;
}
