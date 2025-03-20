#pragma once

#include "utils.h"
#define APPLY_GRAMMAR_SYMBOL(f)                                                \
                                                                               \
    /* Error */ f(Error)                                                       \
                                                                               \
        /* Lexical Symbols */ f(SEMI) f(COMMA) f(ASSIGNOP) f(RELOP) f(PLUS)    \
            f(MINUS) f(STAR) f(DIV) f(AND) f(OR) f(DOT) f(NOT) f(LP) f(RP)     \
                f(LB) f(RB) f(LC) f(RC) f(TYPE) f(STRUCT) f(RETURN) f(IF)      \
                    f(ELSE) f(WHILE) f(INT) f(FLOAT) f(ID)                     \
                                                                               \
        /* Syntax Symbols */ f(Program) f(ExtDefList) f(ExtDef) f(ExtDecList)  \
            f(Specifier) f(StructSpecifier) f(OptTag) f(Tag) f(VarDec)         \
                f(FunDec) f(VarList) f(ParamDec) f(CompSt) f(StmtList) f(Stmt) \
                    f(DefList) f(Def) f(DecList) f(Dec) f(Exp) f(Args)

// enum like GI_SEMI, etc
#define ENUM_GRAMMAR_SYMBOL_AID(x) CONCATENATE(GS_, x),
typedef enum GrammarSymbol {
    APPLY_GRAMMAR_SYMBOL(ENUM_GRAMMAR_SYMBOL_AID)
} GrammarSymbol;
