%{
#include <stdio.h>
int yyerror(const char *s);
int yylex(void);
%}

%%
start: ;
%%

int yyerror(const char *s) {
    fprintf(stderr, "Error: %s\n", s);
    return 1;
}
int yylex() {
    return 0;
}
