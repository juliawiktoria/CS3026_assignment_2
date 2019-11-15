# CS3026 Operating Systems - Assignment 2
# author: Julia Wiktoria Zieba
# ID: 51768280

# makefile for compiling the solution

all : shell.o filesys.o
	gcc -o myprog shell.o filesys.o 

shell.o : shell.c filesys.h
	gcc -c shell.c

mymemory.o : filesys.c filesys.h
	gcc -c filesys.c

clean:
	rm -f shell.o filesys.o 
	
