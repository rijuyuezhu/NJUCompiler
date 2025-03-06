%code requires {
#include "task_engine.h"
}
%{
#include "ast.h"
#include "lex.yy.c"
#include "utils.h"
#include <stdio.h>
extern bool has_lexical_err;
static void yyerror(TaskEngine *engine, char *msg) {
    if (!has_lexical_err) { // lexical error has higher priority
        printf("Error type B at line %d: %s.\n", yylloc.first_line, msg);
    } else {
        has_lexical_err = false;
    }
    engine->ast_error = true;
}

#define SYNTAX_BASIC_ACTION(name, line_no)                                     \
    ({ NSCALL(AstNode, creheap_inner, /, line_no, STRINGIFY(name)); })

#define SYNTAX_BASIC_ACTION1(name, line_no, arg1)                              \
    ({                                                                         \
        AstNode *MPROT(n) =                                                    \
            NSCALL(AstNode, creheap_inner, /, line_no, STRINGIFY(name));       \
        CALL(VecPtr, MPROT(n)->children, reserve, /, 1);                       \
        NSCALL(AstNode, add_child, /, MPROT(n), arg1);                         \
        MPROT(n);                                                              \
    })

#define SYNTAX_BASIC_ACTION2(name, line_no, arg1, arg2)                        \
    ({                                                                         \
        AstNode *MPROT(n) =                                                    \
            NSCALL(AstNode, creheap_inner, /, line_no, STRINGIFY(name));       \
        CALL(VecPtr, MPROT(n)->children, reserve, /, 2);                       \
        NSCALL(AstNode, add_child, /, MPROT(n), arg1);                         \
        NSCALL(AstNode, add_child, /, MPROT(n), arg2);                         \
        MPROT(n);                                                              \
    })

#define SYNTAX_BASIC_ACTION3(name, line_no, arg1, arg2, arg3)                  \
    ({                                                                         \
        AstNode *MPROT(n) =                                                    \
            NSCALL(AstNode, creheap_inner, /, line_no, STRINGIFY(name));       \
        CALL(VecPtr, MPROT(n)->children, reserve, /, 3);                       \
        NSCALL(AstNode, add_child, /, MPROT(n), arg1);                         \
        NSCALL(AstNode, add_child, /, MPROT(n), arg2);                         \
        NSCALL(AstNode, add_child, /, MPROT(n), arg3);                         \
        MPROT(n);                                                              \
    })

#define SYNTAX_BASIC_ACTION4(name, line_no, arg1, arg2, arg3, arg4)            \
    ({                                                                         \
        AstNode *MPROT(n) =                                                    \
            NSCALL(AstNode, creheap_inner, /, line_no, STRINGIFY(name));       \
        CALL(VecPtr, MPROT(n)->children, reserve, /, 4);                       \
        NSCALL(AstNode, add_child, /, MPROT(n), arg1);                         \
        NSCALL(AstNode, add_child, /, MPROT(n), arg2);                         \
        NSCALL(AstNode, add_child, /, MPROT(n), arg3);                         \
        NSCALL(AstNode, add_child, /, MPROT(n), arg4);                         \
        MPROT(n);                                                              \
    })

#define SYNTAX_BASIC_ACTION5(name, line_no, arg1, arg2, arg3, arg4, arg5)      \
    ({                                                                         \
        AstNode *MPROT(n) =                                                    \
            NSCALL(AstNode, creheap_inner, /, line_no, STRINGIFY(name));       \
        CALL(VecPtr, MPROT(n)->children, reserve, /, 5);                       \
        NSCALL(AstNode, add_child, /, MPROT(n), arg1);                         \
        NSCALL(AstNode, add_child, /, MPROT(n), arg2);                         \
        NSCALL(AstNode, add_child, /, MPROT(n), arg3);                         \
        NSCALL(AstNode, add_child, /, MPROT(n), arg4);                         \
        NSCALL(AstNode, add_child, /, MPROT(n), arg5);                         \
        MPROT(n);                                                              \
    })

#define SYNTAX_BASIC_ACTION6(name, line_no, arg1, arg2, arg3, arg4, arg5,      \
                             arg6)                                             \
    ({                                                                         \
        AstNode *MPROT(n) =                                                    \
            NSCALL(AstNode, creheap_inner, /, line_no, STRINGIFY(name));       \
        CALL(VecPtr, MPROT(n)->children, reserve, /, 6);                       \
        NSCALL(AstNode, add_child, /, MPROT(n), arg1);                         \
        NSCALL(AstNode, add_child, /, MPROT(n), arg2);                         \
        NSCALL(AstNode, add_child, /, MPROT(n), arg3);                         \
        NSCALL(AstNode, add_child, /, MPROT(n), arg4);                         \
        NSCALL(AstNode, add_child, /, MPROT(n), arg5);                         \
        NSCALL(AstNode, add_child, /, MPROT(n), arg6);                         \
        MPROT(n);                                                              \
    })

#define SYNTAX_BASIC_ACTION7(name, line_no, arg1, arg2, arg3, arg4, arg5,      \
                             arg6, arg7)                                       \
    ({                                                                         \
        AstNode *MPROT(n) =                                                    \
            NSCALL(AstNode, creheap_inner, /, line_no, STRINGIFY(name));       \
        CALL(VecPtr, MPROT(n)->children, reserve, /, 7);                       \
        NSCALL(AstNode, add_child, /, MPROT(n), arg1);                         \
        NSCALL(AstNode, add_child, /, MPROT(n), arg2);                         \
        NSCALL(AstNode, add_child, /, MPROT(n), arg3);                         \
        NSCALL(AstNode, add_child, /, MPROT(n), arg4);                         \
        NSCALL(AstNode, add_child, /, MPROT(n), arg5);                         \
        NSCALL(AstNode, add_child, /, MPROT(n), arg6);                         \
        NSCALL(AstNode, add_child, /, MPROT(n), arg7);                         \
        MPROT(n);                                                              \
    })
%}

%define api.value.type { typeof(AstNode*) }
%parse-param { typeof(TaskEngine) *engine }
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
    : ExtDefList { engine->ast_root = SYNTAX_BASIC_ACTION1(Program, @$.first_line, $1); }
    ;

ExtDefList
    : ExtDef ExtDefList { $$ = SYNTAX_BASIC_ACTION2(ExtDefList, @$.first_line, $1, $2); }
    | /* empty */ { $$ = NULL; }
    ;

ExtDef
    : Specifier ExtDecList TK_SEMI { $$ = SYNTAX_BASIC_ACTION3(ExtDef, @$.first_line, $1, $2, $3); }
    | Specifier TK_SEMI { $$ = SYNTAX_BASIC_ACTION2(ExtDef, @$.first_line, $1, $2); }
    | Specifier FunDec CompSt { $$ = SYNTAX_BASIC_ACTION3(ExtDef, @$.first_line, $1, $2, $3); }
    | Specifier error CompSt
    | Specifier error TK_SEMI

ExtDecList
    : VarDec { $$ = SYNTAX_BASIC_ACTION1(ExtDecList, @$.first_line, $1); }
    | VarDec TK_COMMA ExtDecList { $$ = SYNTAX_BASIC_ACTION3(ExtDecList, @$.first_line, $1, $2, $3); }
    ;

/* Specifiers */

Specifier
    : TK_TYPE { $$ = SYNTAX_BASIC_ACTION1(Specifier, @$.first_line, $1); }
    | StructSpecifier { $$ = SYNTAX_BASIC_ACTION1(Specifier, @$.first_line, $1); }
    ;

StructSpecifier
    : TK_STRUCT OptTag TK_LC DefList TK_RC { $$ = SYNTAX_BASIC_ACTION5(StructSpecifier, @$.first_line, $1, $2, $3, $4, $5); }
    | TK_STRUCT Tag { $$ = SYNTAX_BASIC_ACTION2(StructSpecifier, @$.first_line, $1, $2); }
    | TK_STRUCT OptTag TK_LC error TK_RC
    ;

OptTag
    : TK_ID { $$ = SYNTAX_BASIC_ACTION1(OptTag, @$.first_line, $1); }
    | /* empty */ { $$ = NULL; }
    ;

Tag
    : TK_ID { $$ = SYNTAX_BASIC_ACTION1(Tag, @$.first_line, $1); }
    ;

/* Declarators */

VarDec
    : TK_ID { $$ = SYNTAX_BASIC_ACTION1(VarDec, @$.first_line, $1); }
    | VarDec TK_LB TK_INT TK_RB { $$ = SYNTAX_BASIC_ACTION4(VarDec, @$.first_line, $1, $2, $3, $4); }
    | VarDec TK_LB error TK_RB
    ;

FunDec
    : TK_ID TK_LP VarList TK_RP { $$ = SYNTAX_BASIC_ACTION4(FunDec, @$.first_line, $1, $2, $3, $4); }
    | TK_ID TK_LP TK_RP { $$ = SYNTAX_BASIC_ACTION3(FunDec, @$.first_line, $1, $2, $3); }
    | TK_ID TK_LP error TK_RP
    | error TK_LP VarList TK_RP
    ;

VarList
    : ParamDec TK_COMMA VarList { $$ = SYNTAX_BASIC_ACTION3(VarList, @$.first_line, $1, $2, $3); }
    | ParamDec { $$ = SYNTAX_BASIC_ACTION1(VarList, @$.first_line, $1); }
    ;

ParamDec
    : Specifier VarDec { $$ = SYNTAX_BASIC_ACTION2(ParamDec, @$.first_line, $1, $2); }
    ;

/* Statements */

CompSt
    : TK_LC DefList StmtList TK_RC { $$ = SYNTAX_BASIC_ACTION4(CompSt, @$.first_line, $1, $2, $3, $4); }
    | error TK_RC
    ;

StmtList
    : Stmt StmtList { $$ = SYNTAX_BASIC_ACTION2(StmtList, @$.first_line, $1, $2); }
    | /* empty */ { $$ = NULL; }
    ;

Stmt
    : Exp TK_SEMI  { $$ = SYNTAX_BASIC_ACTION2(Stmt, @$.first_line, $1, $2); }
    | CompSt { $$ = SYNTAX_BASIC_ACTION1(Stmt, @$.first_line, $1); }
    | TK_RETURN Exp TK_SEMI { $$ = SYNTAX_BASIC_ACTION3(Stmt, @$.first_line, $1, $2, $3); }
    | TK_IF TK_LP Exp TK_RP Stmt %prec LOWER_THAN_ELSE { $$ = SYNTAX_BASIC_ACTION5(Stmt, @$.first_line, $1, $2, $3, $4, $5); }
    | TK_IF TK_LP Exp TK_RP Stmt TK_ELSE Stmt { $$ = SYNTAX_BASIC_ACTION7(Stmt, @$.first_line, $1, $2, $3, $4, $5, $6, $7); }
    | TK_WHILE TK_LP Exp TK_RP Stmt { $$ = SYNTAX_BASIC_ACTION5(Stmt, @$.first_line, $1, $2, $3, $4, $5); }
    | TK_IF error TK_RP Stmt %prec LOWER_THAN_ELSE
    | TK_IF error TK_RP Stmt TK_ELSE Stmt
    ;

/* Local Definitions */

DefList
    : Def DefList { $$ = SYNTAX_BASIC_ACTION2(DefList, @$.first_line, $1, $2); }
    | /* empty */ { $$ = NULL; }
    ;

Def
    : Specifier DecList TK_SEMI { $$ = SYNTAX_BASIC_ACTION3(Def, @$.first_line, $1, $2, $3); }
    | Specifier error TK_SEMI
    ;

DecList
    : Dec { $$ = SYNTAX_BASIC_ACTION1(DecList, @$.first_line, $1); }
    | Dec TK_COMMA DecList { $$ = SYNTAX_BASIC_ACTION3(DecList, @$.first_line, $1, $2, $3); }
    ;

Dec
    : VarDec { $$ = SYNTAX_BASIC_ACTION1(Dec, @$.first_line, $1); }
    | VarDec TK_ASSIGNOP Exp { $$ = SYNTAX_BASIC_ACTION3(Dec, @$.first_line, $1, $2, $3); }
    ;

/* Expressions */

Exp
    : Exp TK_ASSIGNOP Exp { $$ = SYNTAX_BASIC_ACTION3(Exp, @$.first_line, $1, $2, $3); }
    | Exp TK_AND Exp { $$ = SYNTAX_BASIC_ACTION3(Exp, @$.first_line, $1, $2, $3); }
    | Exp TK_OR Exp { $$ = SYNTAX_BASIC_ACTION3(Exp, @$.first_line, $1, $2, $3); } 
    | Exp TK_RELOP Exp { $$ = SYNTAX_BASIC_ACTION3(Exp, @$.first_line, $1, $2, $3); } 
    | Exp TK_PLUS Exp { $$ = SYNTAX_BASIC_ACTION3(Exp, @$.first_line, $1, $2, $3); } 
    | Exp TK_MINUS Exp { $$ = SYNTAX_BASIC_ACTION3(Exp, @$.first_line, $1, $2, $3); } 
    | Exp TK_STAR Exp { $$ = SYNTAX_BASIC_ACTION3(Exp, @$.first_line, $1, $2, $3); } 
    | Exp TK_DIV Exp { $$ = SYNTAX_BASIC_ACTION3(Exp, @$.first_line, $1, $2, $3); } 
    | TK_LP Exp TK_RP { $$ = SYNTAX_BASIC_ACTION3(Exp, @$.first_line, $1, $2, $3); } 
    | TK_MINUS Exp %prec UNARY_MINUS { $$ = SYNTAX_BASIC_ACTION2(Exp, @$.first_line, $1, $2); }
    | TK_NOT Exp { $$ = SYNTAX_BASIC_ACTION2(Exp, @$.first_line, $1, $2); }
    | TK_ID TK_LP Args TK_RP { $$ = SYNTAX_BASIC_ACTION4(Exp, @$.first_line, $1, $2, $3, $4); }
    | TK_ID TK_LP TK_RP { $$ = SYNTAX_BASIC_ACTION3(Exp, @$.first_line, $1, $2, $3); }
    | Exp TK_LB Exp TK_RB { $$ = SYNTAX_BASIC_ACTION4(Exp, @$.first_line, $1, $2, $3, $4); }
    | Exp TK_DOT TK_ID { $$ = SYNTAX_BASIC_ACTION3(Exp, @$.first_line, $1, $2, $3); }
    | TK_ID { $$ = SYNTAX_BASIC_ACTION1(Exp, @$.first_line, $1); }
    | TK_INT { $$ = SYNTAX_BASIC_ACTION1(Exp, @$.first_line, $1); }
    | TK_FLOAT { $$ = SYNTAX_BASIC_ACTION1(Exp, @$.first_line, $1); }
    | error
    ;

Args
    : Exp TK_COMMA Args { $$ = SYNTAX_BASIC_ACTION3(Args, @$.first_line, $1, $2, $3); }
    | Exp { $$ = SYNTAX_BASIC_ACTION1(Args, @$.first_line, $1); }
    ;

%%

