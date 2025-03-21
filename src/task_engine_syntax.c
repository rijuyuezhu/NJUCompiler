#include "ast.h"
#include "syntax.tab.h"
#include "task_engine.h"

extern void yyrestart(FILE *input_file);
extern void yylex_destroy();

void MTD(TaskEngine, parse_ast, /) {
    ASSERT(self->ast_root == NULL, "AST already analyzed");

    FILE *fp = fopen(self->input_file, "r");
    if (!fp) {
        perror(self->input_file);
        exit(EXIT_FAILURE);
    }
    yyrestart(fp);
    yyparse(self);
    yylex_destroy();
    fclose(fp);
}

void MTD(TaskEngine, print_ast, /) {
    ASSERT(self->ast_root != NULL && !self->ast_error,
           "AST not analyzed yet, or error occurred");
    CALL(AstNode, *self->ast_root, print_subtree, /, 0);
}
