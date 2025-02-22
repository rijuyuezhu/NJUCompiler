%option yylineno

%{
#include <stdio.h>
#include "syntax.tab.h"
static int yycolumn = 1;
#define YY_USER_ACTION \
    yylloc.first_line = yylloc.last_line = yylineno; \
    yylloc.first_column = yycolumn; \
    yylloc.last_column = yycolumn + yyleng - 1;\
    yycolumn += yyleng;
%}



%x BLOCKCOMMENT

newline \n
whitespace [ \t\r]+
blockcomment_start "/*"
blockcomment_end "*/"

linecomment "//".*
digit [0-9]
letter_ [a-zA-Z_]

decimal [1-9][0-9]*
octal 0[0-7]*
hexadecimal 0[xX][0-9a-fA-F]+

float_normal [0-9]*\.[0-9]+|[0-9]+\.
float_exp {float_normal}[eE]([+-]?)[0-9]+

ident {letter_}({letter_}|{digit})*

%%

{newline} { yycolumn = 1; }
{linecomment} {}

{blockcomment_start} { BEGIN(BLOCKCOMMENT); }
<BLOCKCOMMENT>{blockcomment_end} { BEGIN(INITIAL); }
<BLOCKCOMMENT>. {}
<BLOCKCOMMENT><<EOF>> { printf("Error type A at Line %d: Missing */ for block comment.\n", yylineno); }

{whitespace} {}
";" { return TK_SEMI; }
"," { return TK_COMMA; }
"=" { return TK_ASSIGNOP; }
>|<|>=|<=|==|!= { return TK_RELOP; }
"+" { return TK_PLUS; }
"-" { return TK_MINUS; }
"*" { return TK_STAR; }
"/" { return TK_DIV; }
"&&" { return TK_AND; }
"||" { return TK_OR; }
"." { return TK_DOT; }
"!" { return TK_NOT; }
"(" { return TK_LP; }
")" { return TK_RP; }
"[" { return TK_LB; }
"]" { return TK_RB; }
"{" { return TK_LC; }
"}" { return TK_RC; }
int|float { return TK_TYPE; }
struct { return TK_STRUCT; }
return { return TK_RETURN; }
if { return TK_IF; }
else { return TK_ELSE; }
while { return TK_WHILE; }

{decimal} { return TK_INT; }
{octal} { return TK_INT; }
{hexadecimal} { return TK_INT; }
{float_exp} { return TK_FLOAT; }
{float_normal} { return TK_FLOAT; }
{ident} { return TK_ID; }

. { printf("Error type A at Line %d: Mystrerious character \"%c\".\n", yylineno, yytext[0]); }
%%
