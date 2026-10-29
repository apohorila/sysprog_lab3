#ifndef AST_H
#define AST_H
#include <stdio.h>

typedef enum {
    N_PROGRAM, N_FUNC, N_PARAM, N_BLOCK, N_DECL, N_ASSIGN, N_IF, N_WHILE,
    N_FOR, N_RETURN, N_PRINT, N_EXPRSTMT, N_BIN, N_UN, N_NUM, N_VAR, N_CALL
} Kind;

typedef struct Node {
    Kind kind;
    char *name;         
    long val;            
    int line;
    struct Node *a, *b, *c, *d;  
    struct Node *next;           
} Node;

Node *mk(Kind k, const char *name, long val, Node *a, Node *b, Node *c, Node *d);
Node *append(Node *list, Node *n);

void ast_json(Node *n, FILE *f);      
void ast_dump(Node *n, int depth);    
int  run_program(Node *root);          
void emit_c(Node *root, FILE *f);      

extern int yylineno;
#endif
