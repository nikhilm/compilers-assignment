/*
 * Lexer for a very simple version of C.
 *
 * Nikhil Marathe - 200801011
 */
%option noyywrap
%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

void tok(const char *type, ...)
{
    va_list argp;
    va_start(argp, type);

    printf("<%s", type);
    if (strcmp(type, "T_DATATYPE") == 0)
        printf(" %s", va_arg(argp, const char *));
    else if (strcmp(type, "T_IDENTIFIER") == 0)
        printf(" %s", va_arg(argp, const char *));
    else if (strcmp(type, "T_INTEGER") == 0)
        printf(" %d", va_arg(argp, int));
    else if (strcmp(type, "T_FLOAT") == 0)
        printf(" %f", va_arg(argp, double));

    printf(">\n");
}
%}

IDENTIFIER [[:alpha:]_][[:alnum:]_]*
INTEGER    [[:digit:]]+
FLOAT      [[:digit:]]?\.[[:digit:]]+

%%
[ \n\r\t\v] /* discard whitespace */

int|INT                tok("T_DATATYPE", "integer");
float|FLOAT            tok("T_DATATYPE", "float");
if                     tok("T_IF");
for                    tok("T_FOR");

"("                    tok("T_LPAREN");
")"                    tok("T_RPAREN");
"{"                    tok("T_LBRACE");
"}"                    tok("T_RBRACE");
"+"                    tok("T_PLUS");
"++"                   tok("T_INCREMENT");
"="                    tok("T_EQ");
"=="                   tok("T_CMP_EQ");
"!="                   tok("T_CMP_NEQ");
"<"                    tok("T_CMP_LT");
"<="                   tok("T_CMP_LTEQ");
">"                    tok("T_CMP_GT");
">="                   tok("T_CMP_GTEQ");
";"                    tok("T_SEMICOLON");

{IDENTIFIER}           tok("T_IDENTIFIER", yytext);
{INTEGER}              tok("T_INTEGER", atoi(yytext));
{FLOAT}                tok("T_FLOAT", atof(yytext));
%%
int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file>\n", argv[0]);
        return 1;
    }

    FILE *f = fopen(argv[1], "r");
    if (!f) {
        fprintf(stderr, "Error opening file %s\n", argv[1]);
        return 1;
    }

    yyin = f;
    yylex();
    return 0;
}
