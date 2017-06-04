#!/bin/bash
./filesystem create 60000 test2
./filesystem mkdir test2 / usr
./filesystem add test2 /usr utils.h
./filesystem add test2 /usr testowy.txt
./filesystem add test2 / utils.h
./filesystem add test2 / testowy.txt

./filesystem rm test2 /usr utils.h
./filesystem rm test2 / utils.h
./filesystem add test2 /usr lena128.bmp
./filesystem add test2 /usr filesystem.c

./filesystem get test2 /usr lena128.bmp

./filesystem fat test2
