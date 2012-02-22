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
    int dot_name; // DOT graph node name
    int visited;
    struct NFAState *next; // so that transitions can be a linked list
                           // transitions[0] is a list of states to which it
                           // can go
} NFAState;

typedef struct DFAState_t {
    struct DFAState *transitions[ALPHABET_SIZE]; // yes it's a colossal waste of space, 0 position is a epsilon transition
    int is_final;
    int dot_name; // DOT graph node name
} DFAState;

NFAState *NFAState_create()
{
    NFAState *state = (NFAState*) malloc(sizeof(NFAState));
    //state->transitions = (NFAState **) calloc(ALPHABET_SIZE, sizeof(NFAState*));
    state->is_final = 0;
    state->dot_name = 0;
    state->visited = 0;
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
    state->dot_name = 0;
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

void postfix_expr_delete(postfix_expr *exp)
{
    free(exp);
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

    postfix_expr *epsilon = postfix_expr_create('\0');
    DL_APPEND(output, epsilon);

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

/******************
 * DOT generation *
 *****************/
int dot_name_counter = 0;
void dot_node(NFAState *node)
{
    if (node->dot_name == 0) {
        node->dot_name = ++dot_name_counter;
        printf("node%d [shape=%s,label=%d];\n", node->dot_name, node->is_final ? "doublecircle" : "circle", node->dot_name);
    }
    // we've already output this node, skip
}

void dot_edge(NFAState *from, NFAState *to, char label)
{
    printf("node%d -> node%d [label=", from->dot_name, to->dot_name);
    if (label == '\0')
        printf("epsilon");
    else
        printf("%c", label);
    printf("]\n");
}

void dot_nfa_rec(NFAState *state)
{
    fprintf(stderr, "dot_nfa_rec: %d\n", state->dot_name);
    if (state->visited)
        return;
    dot_node(state);
    state->visited = 1;
    int i;
    for (i = 0; i < ALPHABET_SIZE; ++i) {
        NFAState *each;
        LL_FOREACH(state->transitions[i], each) {
            dot_nfa_rec(each);
            dot_edge(state, each, (char) i);
        }
    }
}

void dot_nfa(NFAState *start)
{
    printf("digraph nfa {\nsize=\"16,16\";\n");
    // fake start node
    printf("nodestart [shape=point,label=Start];\n");
    dot_nfa_rec(start);
    printf("nodestart -> node%d [label=epsilon];\n", start->dot_name);
    printf("}\n");
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <pattern>\n", argv[0]);
        return 1;
    }

    char *pattern = argv[1];
    NFAState *start_nfa = thompson(pattern);
    dot_nfa(start_nfa);
    //DFAState start_dfa = nfa_to_dfa(start_nfa);
    //graphviz(start_dfa);
    return 0;
}
