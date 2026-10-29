#include <stdlib.h>
#include <string.h>
#include "ast.h"

Node *mk(Kind k, const char *name, long val, Node *a, Node *b, Node *c, Node *d) {
    Node *n = calloc(1, sizeof *n);
    n->kind = k; n->name = name ? strdup(name) : NULL; n->val = val;
    n->a = a; n->b = b; n->c = c; n->d = d; n->line = yylineno;
    return n;
}
Node *append(Node *list, Node *n) {
    if (!list) return n;
    Node *p = list; while (p->next) p = p->next;
    p->next = n; return list;
}


static const char *kname(Kind k) {
    switch (k) {
    case N_PROGRAM: return "Program"; case N_FUNC: return "Func";
    case N_PARAM: return "Param";     case N_BLOCK: return "Block";
    case N_DECL: return "Decl";       case N_ASSIGN: return "Assign";
    case N_IF: return "If";           case N_WHILE: return "While";
    case N_FOR: return "For";         case N_RETURN: return "Return";
    case N_PRINT: return "Print";     case N_EXPRSTMT: return "ExprStmt";
    case N_BIN: return "BinOp";       case N_UN: return "UnOp";
    case N_NUM: return "Num";         case N_VAR: return "Var";
    case N_CALL: return "Call";
    }
    return "?";
}


static int kids(Node *n, Node **out) {
    int c = 0; Node *p;
    switch (n->kind) {
    case N_PROGRAM: for (p = n->a; p; p = p->next) out[c++] = p; break;
    case N_FUNC:    for (p = n->a; p; p = p->next) out[c++] = p;
        out[c++] = n->b; break;
    case N_BLOCK:   for (p = n->a; p; p = p->next) out[c++] = p; break;
    case N_CALL:    for (p = n->a; p; p = p->next) out[c++] = p; break;
    case N_DECL: case N_ASSIGN: case N_RETURN: case N_PRINT: case N_EXPRSTMT: case N_UN:
        if (n->a) { out[c++] = n->a; }
        break;
    case N_BIN: case N_WHILE: out[c++] = n->a; out[c++] = n->b; break;
    case N_IF: out[c++] = n->a; out[c++] = n->b;
        if (n->c) { out[c++] = n->c; }
        break;
    case N_FOR:
        if (n->a) out[c++] = n->a;
        if (n->b) out[c++] = n->b;
        if (n->c) out[c++] = n->c;
        out[c++] = n->d; break;
    default: break;
    }
    return c;
}

static void label(Node *n, char *buf, size_t sz) {
    switch (n->kind) {
    case N_NUM: snprintf(buf, sz, "%ld", n->val); break;
    case N_VAR: case N_PARAM: snprintf(buf, sz, "%s", n->name); break;
    case N_BIN: case N_UN: snprintf(buf, sz, "%s", n->name); break;
    case N_FUNC: case N_CALL: case N_DECL: case N_ASSIGN:
        snprintf(buf, sz, "%s %s", kname(n->kind), n->name); break;
    default: snprintf(buf, sz, "%s", kname(n->kind));
    }
}

static void jstr(const char *s, FILE *f) {
    fputc('"', f);
    for (; *s; s++) { if (*s == '"' || *s == '\\') fputc('\\', f); fputc(*s, f); }
    fputc('"', f);
}

void ast_json(Node *n, FILE *f) {
    Node *k[64]; char buf[128];
    label(n, buf, sizeof buf);
    fprintf(f, "{\"name\":"); jstr(buf, f);
    fprintf(f, ",\"type\":\"%s\",\"line\":%d", kname(n->kind), n->line);
    int c = kids(n, k);
    if (c) {
        fprintf(f, ",\"children\":[");
        for (int i = 0; i < c; i++) { if (i) fputc(',', f); ast_json(k[i], f); }
        fputc(']', f);
    }
    fputc('}', f);
}

void ast_dump(Node *n, int d) {
    Node *k[64]; char buf[128];
    label(n, buf, sizeof buf);
    printf("%*s%s\n", d * 2, "", buf);
    int c = kids(n, k);
    for (int i = 0; i < c; i++) ast_dump(k[i], d + 1);
}


typedef struct { char *name; long val; } Var;
static Var vars[10000]; static int nvars = 0;
static int frame_base = 0;
static int returning = 0; static long retval = 0;
static Node *funcs[256]; static int nfuncs = 0;

static Var *lookup(const char *name) {
    for (int i = nvars - 1; i >= frame_base; i--)
        if (!strcmp(vars[i].name, name)) return &vars[i];
    fprintf(stderr, "runtime error: змінна '%s' не оголошена\n", name); exit(1);
}
static Node *find_func(const char *name) {
    for (int i = 0; i < nfuncs; i++) if (!strcmp(funcs[i]->name, name)) return funcs[i];
    fprintf(stderr, "runtime error: функція '%s' не знайдена\n", name); exit(1);
}

static long eval(Node *n);
static void exec(Node *n);

static long eval(Node *n) {
    switch (n->kind) {
    case N_NUM: return n->val;
    case N_VAR: return lookup(n->name)->val;
    case N_UN:
        if (n->name[0] == '-') return -eval(n->a);
        return !eval(n->a);
    case N_BIN: {
        const char *o = n->name;
        if (!strcmp(o, "&&")) return eval(n->a) && eval(n->b);  
        if (!strcmp(o, "||")) return eval(n->a) || eval(n->b);
        long l = eval(n->a), r = eval(n->b);
        if (!strcmp(o, "+")) return l + r;
        if (!strcmp(o, "-")) return l - r;
        if (!strcmp(o, "*")) return l * r;
        if (!strcmp(o, "/")) { if (!r) { fprintf(stderr, "runtime error: ділення на нуль (рядок %d)\n", n->line); exit(1);} return l / r; }
        if (!strcmp(o, "%")) { if (!r) { fprintf(stderr, "runtime error: ділення на нуль (рядок %d)\n", n->line); exit(1);} return l % r; }
        if (!strcmp(o, "<")) return l < r;
        if (!strcmp(o, ">")) return l > r;
        if (!strcmp(o, "<=")) return l <= r;
        if (!strcmp(o, ">=")) return l >= r;
        if (!strcmp(o, "==")) return l == r;
        if (!strcmp(o, "!=")) return l != r;
        return 0;
    }
    case N_CALL: {
        Node *f = find_func(n->name);
        long args[32]; int na = 0;
        for (Node *a = n->a; a; a = a->next) args[na++] = eval(a);
        int saved_base = frame_base, saved_n = nvars;
        frame_base = nvars;
        int i = 0;
        for (Node *p = f->a; p; p = p->next, i++) {
            if (i >= na) { fprintf(stderr, "runtime error: мало аргументів у %s\n", n->name); exit(1); }
            vars[nvars].name = p->name; vars[nvars++].val = args[i];
        }
        returning = 0; retval = 0;
        exec(f->b);
        long r = retval; returning = 0;
        nvars = saved_n; frame_base = saved_base;
        return r;
    }
    default: fprintf(stderr, "eval: unexpected node\n"); exit(1);
    }
}

static void exec(Node *n) {
    if (!n || returning) return;
    switch (n->kind) {
    case N_BLOCK: {
        int saved = nvars;
        for (Node *s = n->a; s && !returning; s = s->next) exec(s);
        nvars = saved; break;
    }
    case N_DECL: {
        long v = n->a ? eval(n->a) : 0;
        vars[nvars].name = n->name; vars[nvars++].val = v; break;
    }
    case N_ASSIGN: lookup(n->name)->val = eval(n->a); break;
    case N_IF: if (eval(n->a)) exec(n->b); else if (n->c) exec(n->c); break;
    case N_WHILE: while (!returning && eval(n->a)) exec(n->b); break;
    case N_FOR: {
        int saved = nvars;
        if (n->a) exec(n->a);
        while (!returning && (!n->b || eval(n->b))) { exec(n->d); if (n->c) exec(n->c); }
        nvars = saved; break;
    }
    case N_RETURN: retval = eval(n->a); returning = 1; break;
    case N_PRINT: printf("%ld\n", eval(n->a)); break;
    case N_EXPRSTMT: eval(n->a); break;
    default: break;
    }
}

int run_program(Node *root) {
    for (Node *f = root->a; f; f = f->next) funcs[nfuncs++] = f;
    Node call = { N_CALL, "main", 0, 0, NULL, NULL, NULL, NULL, NULL };
    return (int)eval(&call);
}


static void ind(FILE *f, int d) { for (int i = 0; i < d; i++) fputs("    ", f); }
static void ex(Node *n, FILE *f);
static void st(Node *n, FILE *f, int d);

static void ex(Node *n, FILE *f) {
    switch (n->kind) {
    case N_NUM: fprintf(f, "%ld", n->val); break;
    case N_VAR: fputs(n->name, f); break;
    case N_UN: fprintf(f, "(%s", n->name); ex(n->a, f); fputc(')', f); break;
    case N_BIN: fputc('(', f); ex(n->a, f); fprintf(f, " %s ", n->name); ex(n->b, f); fputc(')', f); break;
    case N_CALL:
        fprintf(f, "%s(", n->name);
        for (Node *a = n->a; a; a = a->next) { ex(a, f); if (a->next) fputs(", ", f); }
        fputc(')', f); break;
    default: break;
    }
}
static void simple(Node *n, FILE *f) {   
    if (n->kind == N_DECL) { fprintf(f, "int %s", n->name); if (n->a) { fputs(" = ", f); ex(n->a, f); } }
    else if (n->kind == N_ASSIGN) { fprintf(f, "%s = ", n->name); ex(n->a, f); }
}
static void st(Node *n, FILE *f, int d) {
    switch (n->kind) {
    case N_BLOCK:
        ind(f, d); fputs("{\n", f);
        for (Node *s = n->a; s; s = s->next) st(s, f, d + 1);
        ind(f, d); fputs("}\n", f); break;
    case N_DECL: case N_ASSIGN: ind(f, d); simple(n, f); fputs(";\n", f); break;
    case N_IF:
        ind(f, d); fputs("if (", f); ex(n->a, f); fputs(")\n", f);
        st(n->b, f, n->b->kind == N_BLOCK ? d : d + 1);
        if (n->c) { ind(f, d); fputs("else\n", f); st(n->c, f, n->c->kind == N_BLOCK ? d : d + 1); }
        break;
    case N_WHILE:
        ind(f, d); fputs("while (", f); ex(n->a, f); fputs(")\n", f);
        st(n->b, f, n->b->kind == N_BLOCK ? d : d + 1); break;
    case N_FOR:
        ind(f, d); fputs("for (", f);
        if (n->a) { simple(n->a, f); }
        fputs("; ", f);
        if (n->b) { ex(n->b, f); }
        fputs("; ", f);
        if (n->c) { simple(n->c, f); }
        fputs(")\n", f);
        st(n->d, f, n->d->kind == N_BLOCK ? d : d + 1); break;
    case N_RETURN: ind(f, d); fputs("return ", f); ex(n->a, f); fputs(";\n", f); break;
    case N_PRINT: ind(f, d); fputs("printf(\"%d\\n\", ", f); ex(n->a, f); fputs(");\n", f); break;
    case N_EXPRSTMT: ind(f, d); ex(n->a, f); fputs(";\n", f); break;
    default: break;
    }
}
void emit_c(Node *root, FILE *f) {
    fputs("/* Згенеровано minic */\n#include <stdio.h>\n\n", f);
    for (Node *fn = root->a; fn; fn = fn->next) {       
        fprintf(f, "int %s(", fn->name);
        for (Node *p = fn->a; p; p = p->next) fprintf(f, "int %s%s", p->name, p->next ? ", " : "");
        fputs(");\n", f);
    }
    fputc('\n', f);
    for (Node *fn = root->a; fn; fn = fn->next) {
        fprintf(f, "int %s(", fn->name);
        for (Node *p = fn->a; p; p = p->next) fprintf(f, "int %s%s", p->name, p->next ? ", " : "");
        fputs(")\n", f);
        st(fn->b, f, 0); fputc('\n', f);
    }
}
