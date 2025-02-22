%option yylineno

%{
#include "ast.h"
#include "syntax.tab.h"
#include <stdio.h>
static int yycolumn = 1;
#define YY_USER_ACTION                                                         \
    yylloc.first_line = yylloc.last_line = yylineno;                           \
    yylloc.first_column = yycolumn;                                            \
    yylloc.last_column = yycolumn + yyleng - 1;                                \
    yycolumn += yyleng;

#define LEX_CREATE_ASTNODE(kind, name, ...)                                    \
    ({                                                                         \
        String MPROT(s) = NSCALL(String, from_raw, /, name);                   \
        NSCALL(AstNode, CONCATENATE(creheap_, kind), /, yylineno, MPROT(s),    \
               ##__VA_ARGS__);                                                 \
    })
#define LEX_CREATE_ASTNODE_STR(name, attr)                                     \
    ({                                                                         \
        String MPROT(a) = NSCALL(String, from_raw, /, attr);                   \
        LEX_CREATE_ASTNODE(string, name, MPROT(a));                            \
    })

#define LEX_BASIC_ACTION(name)                                                 \
    ({                                                                         \
        yylval = LEX_CREATE_ASTNODE(basic, STRINGIFY(name));                   \
        return CONCATENATE(TK_, name);                                         \
    })
static int parse_to_int(const char *str) {
    int ret;
    sscanf(str, "%i", &ret);
    return ret;
}
static float parse_to_float(const char *str) {
    float ret;
    sscanf(str, "%f", &ret);
    return ret;
}
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

<BLOCKCOMMENT>{newline} { yycolumn = 1; }
<BLOCKCOMMENT>{blockcomment_end} { BEGIN(INITIAL); }
<BLOCKCOMMENT>. {}
<BLOCKCOMMENT><<EOF>> { printf("Error type A at Line %d: Missing */ for block comment.\n", yylineno); }

{newline} { yycolumn = 1; }
{linecomment} {}
{blockcomment_start} { BEGIN(BLOCKCOMMENT); }

{whitespace} {}
";" { LEX_BASIC_ACTION(SEMI); }
"," { LEX_BASIC_ACTION(COMMA); }
"=" { LEX_BASIC_ACTION(ASSIGNOP); }
>|<|>=|<=|==|!= { LEX_BASIC_ACTION(RELOP); }
"+" { LEX_BASIC_ACTION(PLUS); }
"-" { LEX_BASIC_ACTION(MINUS); }
"*" { LEX_BASIC_ACTION(STAR); }
"/" { LEX_BASIC_ACTION(DIV); }
"&&" { LEX_BASIC_ACTION(AND); }
"||" { LEX_BASIC_ACTION(OR); }
"." { LEX_BASIC_ACTION(DOT); }
"!" { LEX_BASIC_ACTION(NOT); }
"(" { LEX_BASIC_ACTION(LP); }
")" { LEX_BASIC_ACTION(RP); }
"[" { LEX_BASIC_ACTION(LB); }
"]" { LEX_BASIC_ACTION(RB); }
"{" { LEX_BASIC_ACTION(LC); }
"}" { LEX_BASIC_ACTION(RC); }
struct { LEX_BASIC_ACTION(STRUCT); }
return { LEX_BASIC_ACTION(RETURN); }
if { LEX_BASIC_ACTION(IF); }
else { LEX_BASIC_ACTION(ELSE); }
while { LEX_BASIC_ACTION(WHILE); }
int|float { yylval = LEX_CREATE_ASTNODE_STR("TYPE", yytext); return TK_TYPE; }

{decimal}|{octal}|{hexadecimal} { yylval = LEX_CREATE_ASTNODE(int, "INT", parse_to_int(yytext)); return TK_INT; }
{float_exp}|{float_normal} { yylval = LEX_CREATE_ASTNODE(float, "FLOAT", parse_to_float(yytext)); return TK_FLOAT; }
{ident} { yylval = LEX_CREATE_ASTNODE_STR("ID", yytext); return TK_ID; }

. { printf("Error type A at Line %d: Mystrerious character \"%d\".\n", yylineno, (int)yytext[0]); }
%%
