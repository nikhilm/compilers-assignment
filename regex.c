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

/*************************************
 * Regex infix to postfix conversion *
 ************************************/
typedef struct postfix_expr {
    char value;
    struct postfix_expr *next, *prev;
} postfix_expr;

postfix_expr *postfix_expr_create(char value)
{
    postfix_expr *item = (postfix_expr*) malloc(sizeof(postfix_expr));
    item->value = value;
    return item;
}

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
    postfix_expr *ch = postfix_expr_create(op);
    LL_PREPEND(opstack, ch);
    return opstack;
}

/* caller should free */
postfix_expr * postfix(const char *infix)
{
    postfix_expr *output = NULL;
    postfix_expr *opstack = NULL; // its a stack so use it that way, opstack is always the TOP

    int prev_was_op = 0;

    while (*infix != '\0') {
        switch (*infix) {
            case '*':
                opstack = push_op(output, opstack, '*');
                opstack = push_op(output, opstack, '.');
                prev_was_op = 1;
                break;
            case '|':
                opstack = push_op(output, opstack, '|');
                prev_was_op = 1;
                break;
            case '(':
                opstack = push_op(output, opstack, '.');
                opstack = push_op(output, opstack, '(');
                prev_was_op = 1;
                break;
            case ')':
                prev_was_op = 1;
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
typedef struct StartAndEnd {
    NFAState *start;
    NFAState *end;
} StartAndEnd;

StartAndEnd single(char value)
{
    NFAState *start = NFAState_create();
    NFAState *end = NFAState_create();
    end->is_final = 1;
    StartAndEnd d = { .start = start, .end = end };

    NFAState *s1 = NFAState_create();
    NFAState *s2 = NFAState_create();
    LL_PREPEND(start->transitions[0], s1);
    LL_PREPEND(s1->transitions[(int)value], s2);
    LL_PREPEND(s2->transitions[0], end);

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
    NFAState *start = NFAState_create();
    NFAState *end = NFAState_create();
    end->is_final = 1;
    StartAndEnd d = { .start = start, .end = end };

    one.end->is_final = two.end->is_final = 0;

    LL_PREPEND(start->transitions[0], one.start);
    LL_PREPEND(one.end->transitions[0], two.start);
    LL_PREPEND(two.end->transitions[0], end);

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

NFAState *thompson(const char *pattern)
{
    postfix_expr *p = postfix(pattern);
    postfix_expr *el;

    NFAStack *stack = NULL;
    NFAStack *new_top = NULL;
    LL_FOREACH(p, el) {
        fprintf(stderr, "> %c\n", el->value);
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
                LL_PREPEND(stack, new_top);
                break;
        }
    }
    fprintf(stderr, "\n");

    assert(stack);
    assert(!stack->next);
    return stack->nfa.start;
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
