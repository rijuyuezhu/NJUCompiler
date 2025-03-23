#pragma once

#include "general_vec.h"
#include "grammar_symbol.h"
#include "str.h"
#include "utils.h"

typedef enum AstNodeKind {
    ASTNodeInvalid,
    AstNodeBasic,
    AstNodeInt,
    ASTNodeFloat,
    ASTNodeString,
    ASTNodeRelop,
    ASTNodeInner,
} AstNodeKind;

typedef enum RelopKind {
    RelopGT,
    RelopLT,
    RelopGE,
    RelopLE,
    RelopEQ,
    RelopNE,
} RelopKind;

typedef struct AstNode {
    AstNodeKind kind;
    int line_no;

    /* Lex/Syntax Info */
    GrammarSymbol grammar_symbol;
    const char *grammar_symbol_str;

    /* Semantic Info */
    usize symtab_idx;
    usize type_idx;

    /* Attribute_value */
    union {
        int int_val;
        float float_val;
        String str_val;
        RelopKind relop_val;
        VecPtr children;
    };
} AstNode;

AstNode *NSMTD(AstNode, creheap_basic, /, int line_no,
               GrammarSymbol grammar_symbol, const char *grammar_symbol_str);
AstNode *NSMTD(AstNode, creheap_int, /, int line_no,
               GrammarSymbol grammar_symbol, const char *grammar_symbol_str,
               int val);
AstNode *NSMTD(AstNode, creheap_float, /, int line_no,
               GrammarSymbol grammar_symbol, const char *grammar_symbol_str,
               float val);
AstNode *NSMTD(AstNode, creheap_string, /, int line_no,
               GrammarSymbol grammar_symbol, const char *grammar_symbol_str,
               String val);
AstNode *NSMTD(AstNode, creheap_relop, /, int line_no,
               GrammarSymbol grammar_symbol, const char *grammar_symbol_str,
               RelopKind val);
AstNode *NSMTD(AstNode, creheap_inner, /, int line_no,
               GrammarSymbol grammar_symbol, const char *grammar_symbol_str);
void MTD(AstNode, add_child, /, AstNode *child);
void MTD(AstNode, print_subtree, /, usize depth);
void MTD(AstNode, drop, /);
