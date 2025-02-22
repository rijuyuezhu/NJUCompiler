#include <stdio.h>
#include <stdlib.h>

#include "ast.h"
#include "syntax.tab.h"

extern FILE *yyin;

extern void yyrestart(FILE *input_file);
extern void yylex_destroy();

void test_syntax() {
    FILE *fp = fopen("../res/temp.cmm", "r");
    if (!fp) {
        perror("test1.cmm");
        exit(EXIT_FAILURE);
    }
    yyrestart(fp);
    AstNode *root = NULL;
    yyparse(&root);
    yylex_destroy();
    fclose(fp);
    DROPOBJHEAP(AstNode, root);
}
