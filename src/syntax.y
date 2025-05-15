%code requires {
#include "task_engine.h"
}
%{
#include <stdio.h>

#include "syntax.h"
#include "lex.yy.c"
#include "utils.h"
#include "op.h"
#include "str.h"

extern bool has_lexical_err;
static void yyerror(TaskEngine *engine, const char *msg) {
    if (!has_lexical_err) { // lexical error has higher priority
        printf("Syntax error at Line %d: %s.\n", yylloc.first_line, msg);
    } else {
        has_lexical_err = false;
    }
    engine->parse_err = true;
}

%}

%parse-param { TaskEngine *engine }
%locations
%define parse.error verbose

%union {
    int int_val;
    ArithopKind arith;
    RelopKind relop;
    String id;
    usize usize_val;
    IRValue ir_value;
    IRStmtBase *ir_stmt_ptr;
}

%token TK_EOL
%token TK_COLON
%token TK_FUNCTION
%token TK_LABEL
%token TK_SHARP
%token<int_val> TK_INT
%token TK_ASSIGN
%token<arith> TK_STAR
%token<arith> TK_AOP
%token TK_IF
%token<relop> TK_RELOP
%token TK_AMP
%token TK_GOTO
%token TK_RETURN
%token TK_DEC
%token TK_ARG
%token TK_CALL
%token TK_PARAM
%token TK_READ
%token TK_WRITE
%token<id> TK_ID

%type<usize_val> Label
%type<usize_val> Var
%type<arith> Aop
%type<ir_value>Value
%type<ir_value>DerefValue
%type<ir_value>CandidateValue
%type<ir_stmt_ptr>Stmt

%destructor { if ($$) { VDROPOBJHEAP(IRStmtBase, $$); $$ = NULL; } } <ir_stmt_ptr>
%destructor { DROPOBJ(String, $$); } <id>

%%

Global
    : OptEols Program
    ;

Program
    : Program Function
    | /* empty */
    ;

Function
    : TK_FUNCTION TK_ID TK_COLON Eols { init_func(engine, $2); }
    | Function TK_PARAM Var Eols { add_param(engine, $3); }
    | Function TK_DEC Var TK_INT Eols { add_dec(engine, $3, $4); }
    | Function TK_ARG CandidateValue Eols { add_arg(engine, $3); }
    | Function TK_LABEL Label TK_COLON Eols { add_label(engine, $3); }
    | Function Stmt Eols { add_stmt(engine, $2); }
    ;

Stmt
    : Var TK_ASSIGN Value { $$ = (IRStmtBase *)CREOBJHEAP(IRStmtAssign, /, $1, $3); }
    | Var TK_ASSIGN DerefValue { $$ = (IRStmtBase *)CREOBJHEAP(IRStmtLoad, /, $1, $3); }
    | DerefValue TK_ASSIGN CandidateValue { $$ = (IRStmtBase *)CREOBJHEAP(IRStmtStore, /, $1, $3); }
    | Var TK_ASSIGN CandidateValue Aop CandidateValue { $$ = (IRStmtBase *)CREOBJHEAP(IRStmtArith, /, $1, $3, $5, $4); }
    | TK_GOTO Label { $$ = (IRStmtBase *)CREOBJHEAP(IRStmtGoto, /, $2); }
    | TK_IF CandidateValue TK_RELOP CandidateValue TK_GOTO Label { $$ = (IRStmtBase *)CREOBJHEAP(IRStmtIf, /, $2, $4, $3, $6); }
    | TK_RETURN CandidateValue { $$ = (IRStmtBase *)CREOBJHEAP(IRStmtReturn, /, $2); }
    | Var TK_ASSIGN TK_CALL TK_ID { $$ = (IRStmtBase *)CREOBJHEAP(IRStmtCall, /, $1, $4, take_arglist(engine)); }
    | TK_READ Var { $$ = (IRStmtBase *)CREOBJHEAP(IRStmtRead, /, $2); }
    | TK_WRITE CandidateValue { $$ = (IRStmtBase *)CREOBJHEAP(IRStmtWrite, /, $2); }
    ;

Value
    : Var { $$ = get_ir_value_from_var(engine, $1); }
    | TK_SHARP TK_INT { $$ = get_ir_value_from_const(engine, $2); }
    | TK_AMP Var { $$ = get_ir_value_from_addr(engine, $2); }
    ;

DerefValue
    : TK_STAR Value { $$ = $2; }
    ;

CandidateValue
    : Value { $$ = $1; }
    | DerefValue { $$ = get_ir_value_from_deref_value(engine, $1); }
    ;

Var
    : TK_ID { $$ = get_var(engine, $1); } 
    ;

Label
    : TK_ID { $$ = get_label(engine, $1); }

Aop
    : TK_AOP { $$ = $1; }
    | TK_STAR { $$ = $1; }
    ;

Eols
    : TK_EOL Eols
    | TK_EOL

OptEols
    : Eols
    | /* empty */
    ;


%%

