#pragma once

#include "utils.h"

#define APPLY_GRAMMAR_SYMBOL_LEXICAL(f)                                        \
    f(SEMI) f(COMMA) f(ASSIGNOP) f(RELOP) f(PLUS) f(MINUS) f(STAR) f(DIV)      \
        f(AND) f(OR) f(DOT) f(NOT) f(LP) f(RP) f(LB) f(RB) f(LC) f(RC) f(TYPE) \
            f(STRUCT) f(RETURN) f(IF) f(ELSE) f(WHILE) f(INT) f(FLOAT) f(ID)

#define APPLY_GRAMMAR_SYMBOL_SYNTAX(f)                                         \
    f(Program) f(ExtDefList) f(ExtDef) f(ExtDecList) f(Specifier)              \
        f(StructSpecifier) f(OptTag) f(Tag) f(VarDec) f(FunDec) f(VarList)     \
            f(ParamDec) f(CompSt) f(StmtList) f(Stmt) f(DefList) f(Def)        \
                f(DecList) f(Dec) f(Exp) f(Args)

#define APPLY_GRAMMAR_SYMBOL(f)                                                \
    f(Invalid) APPLY_GRAMMAR_SYMBOL_LEXICAL(f) APPLY_GRAMMAR_SYMBOL_SYNTAX(f)

// enum like GS_SEMI, etc
#define ENUM_GRAMMAR_SYMBOL_AID(gs) CONCATENATE(GS_, gs),
typedef enum GrammarSymbol {
    APPLY_GRAMMAR_SYMBOL(ENUM_GRAMMAR_SYMBOL_AID)
} GrammarSymbol;
