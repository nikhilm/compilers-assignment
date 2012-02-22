Compilers assignment
====================

[Nikhil Marathe](https://github.com/nikhilm) & [Shreya
Agrawal](https://github.com/shreya-68)

regex.c
-------

A regex to NFA/DFA reducer. It outputs
a [DOT](http://en.wikipedia.org/wiki/DOT_language) graph for the NFA or DFA.

Usage:

    # compile
    make
    # run
    ./assn1 '<regex>'
    # OR to generate a nice image
    ./assn1 '<regex>' | dot -Tpng -o output.png

You will need the [graphviz](http://graphviz.org) package installed for the dot
command to be available.
