#include <time.h>
#include <stdio.h>
#include "filesystem.h"

int main() {
  create_disc(30000, "Mydisk");
  show_fat("Mydisk");
  
  
  return 0;
}
