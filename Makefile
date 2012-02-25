all: assn1 assn2

assn2: simplec.l
	flex -o simplec.yy.c simplec.l
	gcc -o assn2 simplec.yy.c

assn1: regex.c
	gcc -g -Wall -o $@ $<
