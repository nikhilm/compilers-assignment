/*
 * Nikhil Marathe - 200801011
 *
 * This program accepts a regular expression as the first argument
 * and prints out a equivalent NFA and DFA. (Graphviz?)
 * NOTE: Does not understand escape characters
 *
 * TODO: add support for escaping
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "utlist.h"

#define ALPHABET_SIZE 128

typedef struct NFAState {
    struct NFAState *transitions[ALPHABET_SIZE];
    int is_final;
    struct NFAState *next; // so that transitions can be a linked list
                           // transitions[0] is a list of states to which it
                           // can go
} NFAState;

typedef struct DFAState_t {
    struct DFAState *transitions[ALPHABET_SIZE]; // yes it's a colossal waste of space, 0 position is a epsilon transition
    int is_final;
} DFAState;

NFAState *NFAState_create()
{
    NFAState *state = (NFAState*) malloc(sizeof(NFAState));
    //state->transitions = (NFAState **) calloc(ALPHABET_SIZE, sizeof(NFAState*));
    state->is_final = 0;
    return state;
}

void NFAState_delete(NFAState *state)
{
    free(state);
}

DFAState *DFAState_create()
{
    DFAState *state = (DFAState*) malloc(sizeof(DFAState));
    //state->transitions = (DFAState **) calloc(ALPHABET_SIZE, sizeof(DFAState*));
    state->is_final = 0;
    return state;
}

void DFAState_delete(DFAState *state)
{
    free(state);
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <pattern>\n", argv[0]);
        return 1;
    }

    char *pattern = argv[1];
    NFAState *start_nfa = thompson(pattern);
    start_nfa = 0;
    //graphviz(start_nfa);
    //DFAState start_dfa = nfa_to_dfa(start_nfa);
    //graphviz(start_dfa);
    return 0;
}
