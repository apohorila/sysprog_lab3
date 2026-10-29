minic: parser.tab.c lex.yy.c ast.c ast.h
	gcc -Wall -Wno-unused-function -o minic parser.tab.c lex.yy.c ast.c
parser.tab.c parser.tab.h: parser.y
	bison -d -v parser.y
lex.yy.c: lexer.l parser.tab.h
	flex lexer.l
clean:
	rm -f minic parser.tab.* lex.yy.c parser.output out.c out.json
