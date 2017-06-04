#!/bin/bash
echo "Creating directories tree"
echo "/"
echo -e "\t|"
echo -e "\t /usr"
echo -e "\t /bin"
echo -e "\t /home"
echo -e "\t\t|"
echo -e "\t\t /maciej"
echo -e "\t\t\t|"
echo -e "\t\t\t /dokumenty"
echo -e "\t\t\t\t|"
echo -e "\t\t\t\t /new"
echo -e "\t\t /andrzej"

./filesystem create 20000 test1
./filesystem mkdir test1 / usr
./filesystem mkdir test1 / bin
./filesystem mkdir test1 / home
./filesystem mkdir test1 /home/ maciej
./filesystem mkdir test1 /home/ andrzej
./filesystem mkdir test1 /home/maciej/ dokumenty
./filesystem mkdir test1 /home/maciej/dokumenty new
./filesystem add test1 /home/maciej/dokumenty/new testowy.txt
./filesystem add test1 /home/maciej/dokumenty/ filesystem.h
./filesystem add test1 /home/maciej/dokumenty/ utils.h
./filesystem get test1 /home/maciej/dokumenty/new testowy.txt


./filesystem ls test1 /home/maciej/dokumenty
./filesystem ls test1 /
./filesystem ls test1 /home/

./filesystem stat test1 /home/ maciej
./filesystem stat test1 /home/maciej/dokumenty/new testowy.txt
./filesystem fat test1 

./filesystem rm test1 /home/maciej/dokumenty/ filesystem.h
./filesystem stat test1 /home/maciej/dokumenty/ filesystem.h

./filesystem fat test1 

