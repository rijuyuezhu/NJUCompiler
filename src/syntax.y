%{
#include <stdio.h>
#include "lex.yy.c"
void yyerror(char* msg);
%}

%locations

%token TK_SEMI TK_COMMA
%token TK_ASSIGNOP TK_RELOP
%token TK_PLUS TK_MINUS TK_STAR TK_DIV
%token TK_AND TK_OR TK_DOT TK_NOT 
%token TK_LP TK_RP TK_LB TK_RB TK_LC TK_RC
%token TK_TYPE
%token TK_STRUCT TK_RETURN TK_IF TK_ELSE TK_WHILE
%token TK_INT TK_FLOAT TK_ID

%right TK_ASSIGNOP
%left TK_OR
%left TK_AND
%left TK_RELOP
%left TK_PLUS TK_MINUS
%left TK_STAR TK_DIV
%right TK_NOT
%right UNARY_MINUS
%left TK_DOT
%left TK_LP TK_RP
%left TK_LB TK_RB


%nonassoc LOWER_THAN_ELSE
%nonassoc TK_ELSE

%%

/* High-level Definitions */

Program
    : ExtDefList
    ;

ExtDefList
    : ExtDef ExtDefList
    | /* empty */
    ;

ExtDef
    : Specifier ExtDecList TK_SEMI
    | Specifier TK_SEMI
    | Specifier FunDec CompSt

ExtDecList
    : VarDec
    | VarDec TK_COMMA ExtDecList


/* Specifiers */

Specifier
    : TK_TYPE
    | StructSpecifier

StructSpecifier
    : TK_STRUCT OptTag TK_LC DefList TK_RC
    | TK_STRUCT Tag

OptTag
    : TK_ID
    | /* empty */

Tag
    : TK_ID

/* Declarators */

VarDec
    : TK_ID
    | VarDec TK_LB TK_INT TK_RB

FunDec
    : TK_ID TK_LP VarList TK_RP
    | TK_ID TK_LP TK_RP

VarList
    : ParamDec TK_COMMA VarList
    | ParamDec

ParamDec
    : Specifier VarDec

/* Statements */

CompSt
    : TK_LC DefList StmtList TK_RC

StmtList
    : Stmt StmtList
    | /* empty */

Stmt
    : Exp TK_SEMI
    | CompSt
    | TK_RETURN Exp TK_SEMI
    | TK_IF TK_LP Exp TK_RP Stmt %prec LOWER_THAN_ELSE
    | TK_IF TK_LP Exp TK_RP Stmt TK_ELSE Stmt
    | TK_WHILE TK_LP Exp TK_RP Stmt

/* Local Definitions */

DefList
    : Def DefList
    | /* empty */

Def
    : Specifier DecList TK_SEMI

DecList
    : Dec
    | Dec TK_COMMA DecList

Dec
    : VarDec
    | VarDec TK_ASSIGNOP Exp

/* Expressions */

Exp
    : Exp TK_ASSIGNOP Exp
    | Exp TK_AND Exp
    | Exp TK_OR Exp
    | Exp TK_RELOP Exp
    | Exp TK_PLUS Exp
    | Exp TK_MINUS Exp
    | Exp TK_STAR Exp
    | Exp TK_DIV Exp
    | TK_LP Exp TK_RP
    | TK_MINUS Exp %prec UNARY_MINUS
    | TK_NOT Exp
    | TK_ID TK_LP Args TK_RP
    | TK_ID TK_LP TK_RP
    | Exp TK_LB Exp TK_RB
    | Exp TK_DOT TK_ID
    | TK_ID
    | TK_INT
    | TK_FLOAT

Args
    : Exp TK_COMMA Args
    | Exp

%%

void yyerror(char *msg) {
    printf("Error type B at line %d: \n", yylloc.first_line, msg);
}
