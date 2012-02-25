all: assn1 assn2

assn2: simplec.l
	flex -o simplec.yy.c simplec.l
	gcc -o assn2 simplec.yy.c

assn1: regex.c
	gcc -g -Wall -o $@ $<

clean:
	rm -rf assn1 assn1.dSYM nfa.png assn2 assn2.dSYM simplec.yy.c
