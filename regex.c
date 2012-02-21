/*
 * Nikhil Marathe - 200801011
 *
 * This program accepts a regular expression as the first argument
 * and prints out a equivalent NFA and DFA. (Graphviz?)
 */
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define ALPHABET_SIZE 255
struct NFAState_t {
    // TODO
    int is_final;
};

typedef struct NFAState_t NFAState;

struct DFAState_t {
    DFAState_t *transition[ALPHABET_SIZE]; // yes it's a colossal waste of space, 0 position is a epsilon transition
    int is_final;
};

typedef struct DFAState_t DFAState;

NFAState NFAState_create()
{
    NFAState state = (NFAState) malloc(sizeof(NFAState_t));
    // TODO
    state->is_final = false;
    return state;
}

void NFAState_delete(NFAState state)
{
    free(state);
}

DFAState DFAState_create()
{
    DFAState state = (DFAState) malloc(sizeof(DFAState_t));
    memset(state->transition, 0, sizeof(DFAState_t)*ALPHABET_SIZE);
    state->is_final = false;
    return state;
}

void DFAState_delete(DFAState state)
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
    graphviz(start_nfa);
    DFAState *start_dfa = nfa_to_dfa(start_nfa);
    graphviz(start_dfa);
    return 0;
}
