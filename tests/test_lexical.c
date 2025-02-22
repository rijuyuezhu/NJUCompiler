#include <stdio.h>
#include <stdlib.h>

extern FILE *yyin;
extern int yylex();
extern int yylex_destroy();

void test_lexical() {
    FILE *fp = fopen("../res/temp.cmm", "r");
    if (!fp) {
        perror("test1.cmm");
        exit(EXIT_FAILURE);
    }
    yyin = fp;
    int name;
    while ((name = yylex()) != 0) {
        // printf("%d\n", name);
    }
    yylex_destroy();
    fclose(fp);
}
