#include <time.h>
#include <stdio.h>
#include "filesystem.h"

int main() {
  create_disc(60000, "BigDisk");
  make_directory("BigDisk","/", "img");
  add_file("BigDisk", "lena128.bmp", "/img/");
  get_file("BigDisk", "/img", "lena128.bmp");
  show_fat("BigDisk");
  /* create_disc(18000, "Mydisk");
  make_directory("Mydisk", "/", "usr");
  add_file("Mydisk", "test", "/usr");
  make_directory("Mydisk", "/", "etc");
  make_directory("Mydisk", "/", "bin");
  make_directory("Mydisk", "/usr", "maciej");
  make_directory("Mydisk", "/usr", "andrzej");
  make_directory("Mydisk", "/usr/maciej/", "new");
  make_directory("Mydisk", "/usr/maciej/", "andrzej");
  make_directory("Mydisk", "/usr/maciej/new/", "ABC");
  show_directory("Mydisk", "/");
  show_directory("Mydisk", "/usr");
  
   show_directory("Mydisk", "/usr/maciej");
     show_directory("Mydisk", "/usr/maciej/new/");
  stats("Mydisk", "/", "usr");
  stats("Mydisk", "/usr", "test");

  show_fat("Mydisk");
  get_file("Mydisk", "/usr", "test");
 
   show_fat("Mydisk");
   show_directory("Mydisk", "/usr");
   stats("Mydisk", "/usr", "test");*/
  
  return 0;
}
