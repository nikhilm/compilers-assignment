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
#include <ctype.h>

// include the linked list utilities written by someone else
// a struct with a 'next' member can act as a singly linked list
// while a struct with 'prev' and 'next' is doubly linked
#include "utlist.h"

// because we are only dealing with ascii
#define ALPHABET_SIZE 128
#define NO_OF_STATES 128

// this defines what an NFA state is supposed to be
typedef struct NFAState {
    // for ASCII character with code X, transitions[X] is the next state(s)
    // see the last member 'next'
    // 0 position is a epsilon transition
    struct NFAState *transitions[ALPHABET_SIZE]; // yes it's a colossal waste of space,

    // 1 if this is an accepting state
    int is_final;

    // DOT is a file format to describe graphs,
    // which graphviz will read and generate the pretty graphs
    // this is just a number, and then a global counter is
    // incremented to assign each NFAState a unique name
    // this is not important to the core algorithm
    int dot_name; // DOT graph node name

    // again, have we visited this DOT node when rendering
    // not important to algorithm
    int visited;  // DOT rendering visited?

    // Every transition is actually a linked list of NFA states
    // because by definition an NFA can go to multiple states at
    // the same time.
    struct NFAState *next; // so that transitions can be a linked list
                           // transitions[0] is a list of states to which it
                           // can go
} NFAState;

// TODO: This is something you'll have to fix,
// it probably isn't supposed to look like this
// since a DFAState hold's an amalgamation of NFAStates
// and so needs some way to hold various states in it
typedef struct DFAState {
    struct DFAState *transitions[ALPHABET_SIZE]; // yes it's a colossal waste of space,
    NFAState *nfa_states[NO_OF_STATES];
    int no_of_nfa_states;
    int is_final;
    int dot_name; // DOT graph node name
    struct DFAState *next; // so that marked/unmarked can be a linked list
    int visited;  // DOT rendering visited?
} DFAState;

// abstraction to malloc an NFAState
NFAState *NFAState_create()
{
    NFAState *state = (NFAState*) malloc(sizeof(NFAState));
    state->is_final = 0;
    state->dot_name = 0;
    state->visited = 0;
    return state;
}

// free an NFAState
void NFAState_delete(NFAState *state)
{
    free(state);
}

// TODO: again modify this based on your implementation
DFAState *DFAState_create()
{
    DFAState *state = (DFAState*) malloc(sizeof(DFAState));
    state->no_of_nfa_states = 0;
    state->is_final = 0;
    state->dot_name = 0;
    state->visited = 0;
    return state;
}

void DFAState_delete(DFAState *state)
{
    free(state);
}

void DFAState_add_nfa_state(DFAState *state, NFAState *add)
{
    assert(state->no_of_nfa_states < NO_OF_STATES);
    state->nfa_states[state->no_of_nfa_states++] = add;
}

/*************************************
 * Regex infix to postfix conversion *
 ************************************/

//a doubly linked list is used for 
//holding the postfix form of the expression
//value is the current character
typedef struct postfix_expr {
    char value;
    struct postfix_expr *next, *prev;
} postfix_expr;

// similar to NFAState_create
postfix_expr *postfix_expr_create(char value)
{
    postfix_expr *item = (postfix_expr*) malloc(sizeof(postfix_expr));
    item->value = value;
    return item;
}

void postfix_expr_delete(postfix_expr *exp)
{
    free(exp);
}

// abstracts the task of dealing with when to insert concatenation
// you don't need to worry about this, since you will only
// deal with NFAState, let's just say that it works and i'll explain it
// later
postfix_expr * push_op(postfix_expr *output, postfix_expr *opstack, char op)
{
    // handle duplicate '.'
    // handle popping '.' appropriately
    if (op == '.') {
        while (opstack && opstack->value == '*') {
            postfix_expr *ch = postfix_expr_create(opstack->value);
            DL_APPEND(output, ch);
            LL_DELETE(opstack, opstack);
        }
    }
    else if (op == '|') {
        while (opstack && (opstack->value == '*' || opstack->value == '.')) {
            postfix_expr *ch = postfix_expr_create(opstack->value);
            DL_APPEND(output, ch);
            LL_DELETE(opstack, opstack);
        }
    }

    postfix_expr *ch = postfix_expr_create(op);
    LL_PREPEND(opstack, ch);
    return opstack;
}

// returns a DL list containing the postfix expression given an infix
// expression
// caller should free the returned list
postfix_expr * postfix(const char *infix)
{
    postfix_expr *output = NULL;
    postfix_expr *opstack = NULL; // its a stack so use it that way, opstack is always the TOP

    int prev_was_op = 1;

    while (*infix != '\0') {
        switch (*infix) {
            case '*':
                opstack = push_op(output, opstack, '*');
                break;
            case '|':
                opstack = push_op(output, opstack, '|');
                prev_was_op = 1;
                break;
            case '(':
                if (!prev_was_op)
                    opstack = push_op(output, opstack, '.');
                opstack = push_op(output, opstack, '(');
                prev_was_op = 1;
                break;
            case ')':
                while (opstack->value != '(') {
                    postfix_expr *ch = postfix_expr_create(opstack->value);
                    DL_APPEND(output, ch);
                    LL_DELETE(opstack, opstack);
                    assert (opstack != NULL); // parentheses mismatch
                }
                LL_DELETE(opstack, opstack);
                break;
            default:
                if (!prev_was_op)
                    opstack = push_op(output, opstack, '.');
                postfix_expr *ch = postfix_expr_create(*infix);
                DL_APPEND(output, ch);
                prev_was_op = 0;
                break;
        }
        infix++;
    }

    while (opstack) {
        if (opstack->value == '(')
            assert(0); // mismatch
        postfix_expr *ch = postfix_expr_create(opstack->value);
        DL_APPEND(output, ch);
        LL_DELETE(opstack, opstack);
    }

    return output;
}

/*******************************
 * Thompson core and utilities *
 ******************************/
// holds the start and end nodes of a particular transition diagram
// since Thompson is all about hooking up start and end of the three
// basic types, we use this to simplify things.
typedef struct StartAndEnd {
    NFAState *start;
    NFAState *end;
} StartAndEnd;

// epsilon -> char -> epsilon
StartAndEnd single(char value)
{
    NFAState *start = NFAState_create();
    NFAState *end = NFAState_create();
    end->is_final = 1;
    StartAndEnd d = { .start = start, .end = end };

    // this is the canonical form, but far too many states
    //NFAState *s1 = NFAState_create();
    //NFAState *s2 = NFAState_create();
    //LL_PREPEND(start->transitions[0], s1);
    //LL_PREPEND(s1->transitions[(int)value], s2);
    //LL_PREPEND(s2->transitions[0], end);

    LL_PREPEND(start->transitions[(int)value], end);
    return d;
}

StartAndEnd union_(StartAndEnd one, StartAndEnd two)
{
    NFAState *start = NFAState_create();
    NFAState *end = NFAState_create();
    end->is_final = 1;
    StartAndEnd d = { .start = start, .end = end };

    one.end->is_final = two.end->is_final = 0;

    LL_PREPEND(start->transitions[0], one.start);
    LL_PREPEND(start->transitions[0], two.start);

    LL_PREPEND(one.end->transitions[0], end);
    LL_PREPEND(two.end->transitions[0], end);

    return d;
}

StartAndEnd concat(StartAndEnd one, StartAndEnd two)
{
    // canonical implementation
    /*NFAState *start = NFAState_create();
    NFAState *end = NFAState_create();
    end->is_final = 1;
    StartAndEnd d = { .start = start, .end = end };

    one.end->is_final = two.end->is_final = 0;

    LL_PREPEND(start->transitions[0], one.start);
    LL_PREPEND(one.end->transitions[0], two.start);
    LL_PREPEND(two.end->transitions[0], end);*/

    one.end->is_final = 0;
    LL_PREPEND(one.end->transitions[0], two.start);
    StartAndEnd d = { .start = one.start, .end = two.end };
    return d;
}

StartAndEnd star(StartAndEnd one)
{
    NFAState *start = NFAState_create();
    NFAState *end = NFAState_create();
    end->is_final = 1;
    StartAndEnd d = { .start = start, .end = end };

    one.end->is_final = 0;

    LL_PREPEND(start->transitions[0], one.start);
    LL_PREPEND(start->transitions[0], end);

    LL_PREPEND(one.end->transitions[0], one.start);
    LL_PREPEND(one.end->transitions[0], end);

    return d;
}

char inputs[ALPHABET_SIZE];
int no_of_inputs = 0;
void add_to_input(char value)
{
    int i;
    for( i = 0; i < no_of_inputs; i++)
    {
        if(inputs[i] == value)
            return;
    }
    inputs[no_of_inputs] = value;
    no_of_inputs++;
}

// this is used to convert a postfix expression
// to the final complete NFA
// stack starts empty
// any basic NFA form is put on the stack as
// its start and end pair
// on an operation, there is an n-ary pop depending
// on the arity of the operation and then result is pushed
typedef struct NFAStack {
    StartAndEnd nfa;
    struct NFAStack *next;
} NFAStack;

NFAStack *NFAStack_create(StartAndEnd value)
{
    NFAStack *item = (NFAStack*) malloc(sizeof(NFAStack));
    item->nfa = value;
    return item;
}

// core Thompson algorithm
NFAState *thompson(const char *pattern)
{
    postfix_expr *p = postfix(pattern);
    postfix_expr *el;

    NFAStack *stack = NULL;
    NFAStack *new_top = NULL;
    LL_FOREACH(p, el) {
        switch (el->value) {
            case '*':
                assert(stack);
                new_top = NFAStack_create(star(stack->nfa));
                LL_DELETE(stack, stack);
                LL_PREPEND(stack, new_top);
                break;
            case '.':
                assert(stack);
                assert(stack->next);
                new_top = NFAStack_create(concat(stack->next->nfa, stack->nfa));
                LL_DELETE(stack, stack);
                LL_DELETE(stack, stack);
                LL_PREPEND(stack, new_top);
                break;
            case '|':
                assert(stack);
                assert(stack->next);
                new_top = NFAStack_create(union_(stack->next->nfa, stack->nfa));
                LL_DELETE(stack, stack);
                LL_DELETE(stack, stack);
                LL_PREPEND(stack, new_top);
                break;
            default:
                new_top = NFAStack_create(single(el->value));
                add_to_input(el->value);
                LL_PREPEND(stack, new_top);
                break;
        }
    }

    assert(stack);
    assert(!stack->next);
    return stack->nfa.start;
}

// converting NFA to DFA

int find_closure(DFAState *dfa_node)
{
   NFAState *n;
   NFAState *nfa_node;
   int i;
   for( i = 0; i < dfa_node->no_of_nfa_states; i++)
   {
       nfa_node = dfa_node->nfa_states[i];

       LL_FOREACH(nfa_node->transitions[0], n)
       {
           DFAState_add_nfa_state(dfa_node, n);
       }
    }
   return 1;
}

int same_state(DFAState *stateA, DFAState *stateB)
{
    if (stateA->no_of_nfa_states != stateB->no_of_nfa_states)
        return 0;

    int i, j;
    for (i = 0; i < stateA->no_of_nfa_states; i++) {
        NFAState *nfaState = stateA->nfa_states[i];
        int inB = 0;
        for (j = 0; j < stateB->no_of_nfa_states; j++) {
            if (nfaState == stateB->nfa_states[j]) {
                inB = 1;
                break;
            }
        }

        if (!inB)
            return 0;
    }
    return 1;
}

DFAState *exists(DFAState *states, DFAState *new_state)
{
    DFAState *state;
    LL_FOREACH(states, state)
    {
        if (same_state(state, new_state)) {
            return state;
        }
    }
    return NULL;
}

DFAState *nfa_to_dfa(NFAState *nfa_start)
{
    DFAState *unmarked = NULL;
    DFAState *marked = NULL;
    DFAState *start = DFAState_create();
    DFAState_add_nfa_state(start, nfa_start);
    find_closure(start);
    int k;
    for (k = 0; k < start->no_of_nfa_states; k++) {
        if (start->nfa_states[k]->is_final) {
            start->is_final = 1;
            break;
        }
    }

    LL_PREPEND(unmarked, start);

    int i,j,x;
    NFAState *n;
    while (unmarked != NULL) {
        DFAState *first = unmarked;
        LL_DELETE(unmarked, first);
        // mark the state
        LL_PREPEND(marked, first);
        for( i = 0; i < no_of_inputs; i++)
        {
            char input = inputs[i];

            DFAState *new_state = DFAState_create();
            x = first->no_of_nfa_states;
            for( j = 0; j < x; j++)
            {
                LL_FOREACH( first->nfa_states[j]->transitions[(int)input], n)
                {
                    DFAState_add_nfa_state(new_state, n);
                }
            }
            find_closure(new_state);
            int k;
            for (k = 0; k < new_state->no_of_nfa_states; k++) {
                if (new_state->nfa_states[k]->is_final) {
                    new_state->is_final = 1;
                    break;
                }
            }

            DFAState *existing = exists(unmarked, new_state);
            if (!existing)
                existing = exists(marked, new_state);

            if (existing) {
                DFAState_delete(new_state);
                first->transitions[(int)input] = existing;
            }
            else {
                LL_PREPEND(unmarked, new_state);
                first->transitions[(int)input] = new_state;
            }

        }
    }

    return start;
}

/******************
 * DOT generation *
 *****************/
// the global counter for assigning DOT node labels
int dot_name_counter = 0;
// output a DOT description of an NFAState,
// you'll write something similar for DFA
void dot_node(NFAState *node)
{
    if (node->dot_name == 0) {
        node->dot_name = ++dot_name_counter;
        printf("node%d [shape=%s,label=%d];\n", node->dot_name, node->is_final ? "doublecircle" : "circle", node->dot_name);
    }
    // we've already output this node, skip
}

// output a DOT description of a transition
void dot_edge(NFAState *from, NFAState *to, char label)
{
    printf("node%d -> node%d [label=", from->dot_name, to->dot_name);
    if (label == '\0') {
        printf("epsilon");
    }
    else if(isspace(label)) {
        switch (label) {
            case ' ':
                printf("sp");
                break;
            case '\t':
                printf("tab");
                break;
            case '\n':
                printf("nl");
                break;
            case '\r':
                printf("cr");
                break;
            default:
                printf("whitespace");
        }
    }
    else {
        printf("%c", label);
    }
    printf("]\n");
}

// recursively output a complete NFA
void dot_nfa_rec(NFAState *state)
{
    // ignore visited
    if (state->visited)
        return;
    // output node
    dot_node(state);
    // now visited
    state->visited = 1;
    int i;
    // follow all transitions and output them
    for (i = 0; i < ALPHABET_SIZE; ++i) {
        NFAState *each;
        LL_FOREACH(state->transitions[i], each) {
            dot_nfa_rec(each);
            // draw an edge from state to this transition
            dot_edge(state, each, (char) i);
        }
    }
}

// prints out DOT prologue and epilogue
// then calls the recursive algorithm
void dot_nfa(NFAState *start)
{
    printf("digraph nfa {\nsize=\"16,16\";\n");
    // fake start node
    printf("nodestart [shape=point,label=Start];\n");
    dot_nfa_rec(start);
    printf("nodestart -> node%d [label=Start];\n", start->dot_name);
    printf("}\n");
}

void dot_dfa_node(DFAState *node)
{
    if (node->dot_name == 0) {
        node->dot_name = ++dot_name_counter;
        printf("node%d [shape=%s,label=%d];\n", node->dot_name, node->is_final ? "doublecircle" : "circle", node->dot_name);
    }
    // we've already output this node, skip
}

// output a DOT description of a transition
void dot_dfa_edge(DFAState *from, DFAState *to, char label)
{
    printf("node%d -> node%d [label=", from->dot_name, to->dot_name);
    if (label == '\0') {
        printf("epsilon");
    }
    else if(isspace(label)) {
        switch (label) {
            case ' ':
                printf("sp");
                break;
            case '\t':
                printf("tab");
                break;
            case '\n':
                printf("nl");
                break;
            case '\r':
                printf("cr");
                break;
            default:
                printf("whitespace");
        }
    }
    else {
        printf("%c", label);
    }
    printf("]\n");
}
void dot_dfa_rec(DFAState *state)
{
    if (state->visited)
        return;
    dot_dfa_node(state);
    state->visited = 1;
    int i;
    for (i = 0; i < ALPHABET_SIZE; ++i) {
        if (state->transitions[i] != NULL) {
            dot_dfa_rec(state->transitions[i]);
            dot_dfa_edge(state, state->transitions[i], (char) i);
        }
    }
}

void dot_dfa(DFAState *start)
{
    printf("digraph dfa {\nsize=\"16,16\";\n");
    // fake start node
    printf("nodestart [shape=point,label=Start];\n");
    dot_dfa_rec(start);
    printf("nodestart -> node%d [label=Start];\n", start->dot_name);
    printf("}\n");
}

int main(int argc, char **argv)
{
    int output_dfa = 0;
    if (argc >= 2 && strcmp(argv[1], "--dfa") == 0) {
        output_dfa = 1;
        int i;
        for (i = 1; i < argc-1; i++)
            argv[i] = argv[i+1];
        argc--;
    }

    if (argc < 2) {
        fprintf(stderr, "Usage: %s [--dfa] <pattern>\n", argv[0]);
        return 1;
    }

    char *pattern = argv[1];
    NFAState *start_nfa = thompson(pattern);

    if (output_dfa) {
        DFAState *start_dfa = nfa_to_dfa(start_nfa);
        dot_dfa(start_dfa);
    }
    else {
        dot_nfa(start_nfa);
    }
    return 0;
}
