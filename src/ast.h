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
    usize production_id;

    /* Semantic Info */
    usize symtab_idx;
    usize type_idx;

    // only maybe non-null for grammar_symbols below:
    // 1. StructSpecifier and its father (Specifier with prod_id 1); the struct
    // name
    // 2. ExpDecList, ParamDec, Dec; the declared var name
    // ---
    // 3. FunDec; the declared func
    // 4. Exp (prod_id 11, 12 -- call exprs); the called func name
    // 5. Exp (prod_id 14 -- field access); the field name
    // 6. Exp (prod_id 15 -- Exp -> ID); the id name
    //
    // The last four are used in IR generation
    struct SymbolEntry *symentry_ptr;

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
               GrammarSymbol grammar_symbol, int production_id);
AstNode *NSMTD(AstNode, creheap_int, /, int line_no,
               GrammarSymbol grammar_symbol, int production_id, int val);
AstNode *NSMTD(AstNode, creheap_float, /, int line_no,
               GrammarSymbol grammar_symbol, int production_id, float val);
AstNode *NSMTD(AstNode, creheap_string, /, int line_no,
               GrammarSymbol grammar_symbol, int production_id, String val);
AstNode *NSMTD(AstNode, creheap_relop, /, int line_no,
               GrammarSymbol grammar_symbol, int production_id, RelopKind val);
AstNode *NSMTD(AstNode, creheap_inner, /, int line_no,
               GrammarSymbol grammar_symbol, int production_id);
void MTD(AstNode, add_child, /, AstNode *child);
void MTD(AstNode, print_subtree, /, usize depth);
void MTD(AstNode, drop, /);
