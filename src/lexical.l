%option yylineno
%{
#include <stdio.h>
#include <string.h>

#include "syntax.tab.h"

static int yycolumn = 1;
bool has_lexical_err = false;
#define YY_USER_ACTION                                                         \
    yylloc.first_line = yylloc.last_line = yylineno;                           \
    yylloc.first_column = yycolumn;                                            \
    yylloc.last_column = yycolumn + yyleng - 1;                                \
    yycolumn += yyleng;

static int parse_to_int(const char *str) {
    char *endptr;
    long ret = strtol(str, &endptr, 10);
    if (*endptr != '\0') {
        printf("Lexical error at Line %d: \"%s\" is not a valid integer.\n",
               yylineno, str);
        has_lexical_err = true;
        return 0;
    }
    return (int)ret;
}

static RelopKind parse_to_relop(const char *str) {
    usize len = strlen(str);
    ASSERT(1 <= len && len <= 2);
    if (len == 1) {
        if (str[0] == '>') {
            return RelopGT;
        } else if (str[0] == '<') {
            return RelopLT;
        }
    } else {
        ASSERT(str[1] == '=');
        if (str[0] == '>') {
            return RelopGE;
        } else if (str[0] == '<') {
            return RelopLE;
        } else if (str[0] == '!') {
            return RelopNE;
        } else if (str[0] == '=') {
            return RelopEQ;
        }
    }
    PANIC("Unreachable");
}
%}



newline \n
whitespace [ \t\r]+

digit [0-9]
letter_ [a-zA-Z_]

number -?[1-9][0-9]*|0

ident {letter_}({letter_}|{digit})*

%%

":" { return TK_COLON; }
"FUNCTION" { return TK_FUNCTION; }
"LABEL" { return TK_LABEL; }
"#" { return TK_SHARP; }
{number} { yylval.int_val = parse_to_int(yytext); return TK_INT; }
":=" { return TK_ASSIGN; }
"+" { yylval.arith = ArithopAdd; return TK_AOP; }
"-" { yylval.arith = ArithopSub; return TK_AOP; }
"*" { yylval.arith = ArithopMul; return TK_STAR; }
"/" { yylval.arith = ArithopDiv; return TK_AOP; }
">"|"<"|">="|"<="|"=="|"!=" { yylval.relop = parse_to_relop(yytext); return TK_RELOP; }
"&" { return TK_AMP; }
"IF" { return TK_IF; }
"GOTO" { return TK_GOTO; }
"RETURN" { return TK_RETURN; }
"DEC" { return TK_DEC; }
"ARG" { return TK_ARG; }
"CALL" { return TK_CALL; }
"PARAM" { return TK_PARAM; }
"READ" { return TK_READ; }
"WRITE" { return TK_WRITE; }
{ident} { yylval.id = NSCALL(String, from_raw, /, yytext); return TK_ID; }
{whitespace} {}
{newline}+ { yycolumn = 1; return TK_EOL; }
<<EOF>> { static int once = 0; if (once == 0) { once = 1; return TK_EOL; } else { return 0; } }
. { printf("Lexical error at Line %d: Mysterious character \"%c\".\n", yylineno, yytext[0]); has_lexical_err = true; }
%%
