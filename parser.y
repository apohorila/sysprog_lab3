%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
int yylex(void);
void yyerror(const char *s);
Node *root;
extern int lex_trace;
%}
%define parse.trace

%union { long num; char *str; struct Node *node; }

%token <num> NUMBER
%token <str> ID
%token INT IF ELSE WHILE FOR RETURN PRINT
%token AND OR EQ NE LE GE

%type <node> program func_list func param_list_opt param_list block stmt_list stmt
%type <node> simple_stmt expr arg_list_opt arg_list


%nonassoc LOWER_THAN_ELSE     
%nonassoc ELSE
%left OR
%left AND
%left EQ NE
%left '<' '>' LE GE
%left '+' '-'
%left '*' '/' '%'
%right UMINUS '!'

%%
program
    : func_list                     { root = mk(N_PROGRAM, NULL, 0, $1, NULL, NULL, NULL); }
    ;
func_list
    : func                          { $$ = $1; }
    | func_list func                { $$ = append($1, $2); }
    ;
func
    : INT ID '(' param_list_opt ')' block
                                    { $$ = mk(N_FUNC, $2, 0, $4, $6, NULL, NULL); }
    ;
param_list_opt
    : /* empty */                   { $$ = NULL; }
    | param_list                    { $$ = $1; }
    ;
param_list
    : INT ID                        { $$ = mk(N_PARAM, $2, 0, NULL, NULL, NULL, NULL); }
    | param_list ',' INT ID         { $$ = append($1, mk(N_PARAM, $4, 0, NULL, NULL, NULL, NULL)); }
    ;
block
    : '{' stmt_list '}'             { $$ = mk(N_BLOCK, NULL, 0, $2, NULL, NULL, NULL); }
    ;
stmt_list
    : /* empty */                   { $$ = NULL; }
    | stmt_list stmt                { $$ = append($1, $2); }
    ;
simple_stmt
    : INT ID                        { $$ = mk(N_DECL, $2, 0, NULL, NULL, NULL, NULL); }
    | INT ID '=' expr               { $$ = mk(N_DECL, $2, 0, $4, NULL, NULL, NULL); }
    | ID '=' expr                   { $$ = mk(N_ASSIGN, $1, 0, $3, NULL, NULL, NULL); }
    ;
stmt
    : simple_stmt ';'               { $$ = $1; }
    | expr ';'                      { $$ = mk(N_EXPRSTMT, NULL, 0, $1, NULL, NULL, NULL); }
    | PRINT '(' expr ')' ';'        { $$ = mk(N_PRINT, NULL, 0, $3, NULL, NULL, NULL); }
    | RETURN expr ';'               { $$ = mk(N_RETURN, NULL, 0, $2, NULL, NULL, NULL); }
    | IF '(' expr ')' stmt %prec LOWER_THAN_ELSE
                                    { $$ = mk(N_IF, NULL, 0, $3, $5, NULL, NULL); }
    | IF '(' expr ')' stmt ELSE stmt
                                    { $$ = mk(N_IF, NULL, 0, $3, $5, $7, NULL); }
    | WHILE '(' expr ')' stmt       { $$ = mk(N_WHILE, NULL, 0, $3, $5, NULL, NULL); }
    | FOR '(' simple_stmt ';' expr ';' simple_stmt ')' stmt
                                    { $$ = mk(N_FOR, NULL, 0, $3, $5, $7, $9); }
    | block                         { $$ = $1; }
    ;
expr
    : expr OR expr                  { $$ = mk(N_BIN, "||", 0, $1, $3, NULL, NULL); }
    | expr AND expr                 { $$ = mk(N_BIN, "&&", 0, $1, $3, NULL, NULL); }
    | expr EQ expr                  { $$ = mk(N_BIN, "==", 0, $1, $3, NULL, NULL); }
    | expr NE expr                  { $$ = mk(N_BIN, "!=", 0, $1, $3, NULL, NULL); }
    | expr '<' expr                 { $$ = mk(N_BIN, "<", 0, $1, $3, NULL, NULL); }
    | expr '>' expr                 { $$ = mk(N_BIN, ">", 0, $1, $3, NULL, NULL); }
    | expr LE expr                  { $$ = mk(N_BIN, "<=", 0, $1, $3, NULL, NULL); }
    | expr GE expr                  { $$ = mk(N_BIN, ">=", 0, $1, $3, NULL, NULL); }
    | expr '+' expr                 { $$ = mk(N_BIN, "+", 0, $1, $3, NULL, NULL); }
    | expr '-' expr                 { $$ = mk(N_BIN, "-", 0, $1, $3, NULL, NULL); }
    | expr '*' expr                 { $$ = mk(N_BIN, "*", 0, $1, $3, NULL, NULL); }
    | expr '/' expr                 { $$ = mk(N_BIN, "/", 0, $1, $3, NULL, NULL); }
    | expr '%' expr                 { $$ = mk(N_BIN, "%", 0, $1, $3, NULL, NULL); }
    | '-' expr %prec UMINUS         { $$ = mk(N_UN, "-", 0, $2, NULL, NULL, NULL); }
    | '!' expr                      { $$ = mk(N_UN, "!", 0, $2, NULL, NULL, NULL); }
    | '(' expr ')'                  { $$ = $2; }
    | NUMBER                        { $$ = mk(N_NUM, NULL, $1, NULL, NULL, NULL, NULL); }
    | ID                            { $$ = mk(N_VAR, $1, 0, NULL, NULL, NULL, NULL); }
    | ID '(' arg_list_opt ')'       { $$ = mk(N_CALL, $1, 0, $3, NULL, NULL, NULL); }
    ;
arg_list_opt
    : /* empty */                   { $$ = NULL; }
    | arg_list                      { $$ = $1; }
    ;
arg_list
    : expr                          { $$ = $1; }
    | arg_list ',' expr             { $$ = append($1, $3); }
    ;
%%

void yyerror(const char *s) {
    fprintf(stderr, "syntax error (рядок %d): %s\n", yylineno, s);
}

extern FILE *yyin;
int main(int argc, char **argv) {
    const char *in = NULL, *json = NULL, *cout = NULL; int run = 0, dump = 0;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--run")) run = 1;
        else if (!strcmp(argv[i], "--dump")) dump = 1;
        else if (!strcmp(argv[i], "--trace-lex")) lex_trace = 1;
        else if (!strcmp(argv[i], "--trace-parse")) yydebug = 1;
        else if (!strcmp(argv[i], "--ast") && i + 1 < argc) json = argv[++i];
        else if (!strcmp(argv[i], "--emit-c") && i + 1 < argc) cout = argv[++i];
        else in = argv[i];
    }
    if (!in) { fprintf(stderr, "usage: %s file.mc [--trace-lex] [--trace-parse] [--dump] [--ast out.json] [--emit-c out.c] [--run]\n", argv[0]); return 1; }
    yyin = fopen(in, "r");
    if (!yyin) { perror(in); return 1; }
    if (yyparse() != 0) return 1;
    if (dump) ast_dump(root, 0);
    if (json) { FILE *f = fopen(json, "w"); ast_json(root, f); fclose(f); }
    if (cout) { FILE *f = fopen(cout, "w"); emit_c(root, f); fclose(f); }
    if (run) return run_program(root);
    return 0;
}
