%{
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "parser.tab.h"
int lex_trace = 0;
#define T(tok, s) do { if (lex_trace) fprintf(stderr, "LEX: %s(%s)\n", s, yytext); return tok; } while (0)
%}
%option noyywrap yylineno nounput noinput

%%
"int"        { T(INT, "INT"); }
"if"         { T(IF, "IF"); }
"else"       { T(ELSE, "ELSE"); }
"while"      { T(WHILE, "WHILE"); }
"for"        { T(FOR, "FOR"); }
"return"     { T(RETURN, "RETURN"); }
"print"      { T(PRINT, "PRINT"); }
0[xX][0-9a-fA-F]+ { yylval.num = strtol(yytext, NULL, 16); T(NUMBER, "HEX"); }
[0-9]+       { yylval.num = atol(yytext); T(NUMBER, "NUMBER"); }
[A-Za-z_][A-Za-z0-9_]* { yylval.str = strdup(yytext); T(ID, "ID"); }
"&&"         { T(AND, "AND"); }
"||"         { T(OR, "OR"); }
"=="         { T(EQ, "EQ"); }
"!="         { T(NE, "NE"); }
"<="         { T(LE, "LE"); }
">="         { T(GE, "GE"); }
[-+*/%<>=!(){};,]  { T(yytext[0], "SYMBOL"); }
"//".*       { /* однорядковий коментар */ }
"/*"([^*]|\*+[^*/])*\*+"/"  { /* багаторядковий коментар */ }
[ \t\r\n]+   { /* пропуск */ }
.            { fprintf(stderr, "lexical error (рядок %d): невідомий символ '%s'\n", yylineno, yytext); exit(1); }
%%
