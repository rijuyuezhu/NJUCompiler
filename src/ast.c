#include "ast.h"
#include "debug.h"
#include "grammar_symbol.h"
#include "str.h"
#include "utils.h"

#define TOSTR_GRAMMAR_SYMBOL_AID(gs) [CONCATENATE(GS_, gs)] = STRINGIFY(gs),

static const char *grammar_symbol_str[] = {
    APPLY_GRAMMAR_SYMBOL(TOSTR_GRAMMAR_SYMBOL_AID)};

static AstNode *common_init(int line_no, GrammarSymbol grammar_symbol,
                            int production_id) {
    AstNode *node = CREOBJRAWHEAP(AstNode);
    node->line_no = line_no;
    node->grammar_symbol = grammar_symbol;
    node->production_id = production_id;
    node->symtab_idx = 0;
    node->type_idx = 0;
    node->symentry_ptr = NULL;
    return node;
}
AstNode *NSMTD(AstNode, creheap_basic, /, int line_no,
               GrammarSymbol grammar_symbol, int production_id) {
    AstNode *node = common_init(line_no, grammar_symbol, production_id);
    node->kind = AstNodeBasic;
    return node;
}

AstNode *NSMTD(AstNode, creheap_int, /, int line_no,
               GrammarSymbol grammar_symbol, int production_id, int val) {
    AstNode *node = common_init(line_no, grammar_symbol, production_id);
    node->kind = AstNodeInt;
    node->int_val = val;
    return node;
}

AstNode *NSMTD(AstNode, creheap_float, /, int line_no,
               GrammarSymbol grammar_symbol, int production_id, float val) {
    AstNode *node = common_init(line_no, grammar_symbol, production_id);
    node->kind = ASTNodeFloat;
    node->float_val = val;
    return node;
}

AstNode *NSMTD(AstNode, creheap_string, /, int line_no,
               GrammarSymbol grammar_symbol, int production_id, String val) {
    AstNode *node = common_init(line_no, grammar_symbol, production_id);
    node->kind = ASTNodeString;
    node->str_val = val;
    return node;
}

AstNode *NSMTD(AstNode, creheap_relop, /, int line_no,
               GrammarSymbol grammar_symbol, int production_id, RelopKind val) {
    AstNode *node = common_init(line_no, grammar_symbol, production_id);
    node->kind = ASTNodeRelop;
    node->relop_val = val;
    return node;
}

AstNode *NSMTD(AstNode, creheap_inner, /, int line_no,
               GrammarSymbol grammar_symbol, int production_id) {
    AstNode *node = common_init(line_no, grammar_symbol, production_id);
    node->kind = ASTNodeInner;
    CALL(VecPtr, node->children, init, /);
    return node;
}

void MTD(AstNode, add_child, /, AstNode *child) {
    if (child == NULL) {
        return;
    }
    ASSERT(self->kind == ASTNodeInner, "Cannot add child to non-inner node");
    CALL(VecPtr, self->children, push_back, /, child);
}

void MTD(AstNode, print_subtree, /, usize depth) {
    if (self->kind == ASTNodeInner && self->children.size == 0) {
        return;
    }
    for (usize i = 0; i < depth; i++) {
        printf("  ");
    }
    switch (self->kind) {
    case AstNodeBasic:
    case ASTNodeRelop:
        printf("%s\n", grammar_symbol_str[self->grammar_symbol]);
        break;
    case AstNodeInt:
        printf("%s: %d\n", grammar_symbol_str[self->grammar_symbol],
               self->int_val);
        break;
    case ASTNodeFloat:
        printf("%s: %f\n", grammar_symbol_str[self->grammar_symbol],
               self->float_val);
        break;
    case ASTNodeString:
        printf("%s: %s\n", grammar_symbol_str[self->grammar_symbol],
               STRING_C_STR(self->str_val));
        break;
    case ASTNodeInner:
        printf("%s (%d)\n", grammar_symbol_str[self->grammar_symbol],
               self->line_no);
        for (usize i = 0; i < self->children.size; i++) {
            CALL(AstNode, *(AstNode *)self->children.data[i], print_subtree, /,
                 depth + 1);
        }
        break;
    default:
        PANIC("Invalid AstNodeType");
    }
}

void MTD(AstNode, drop, /) {
    if (self->kind == ASTNodeString) {
        DROPOBJ(String, self->str_val);
    } else if (self->kind == ASTNodeInner) {
        for (usize i = 0; i < self->children.size; i++) {
            DROPOBJHEAP(AstNode, self->children.data[i]);
        }
        DROPOBJ(VecPtr, self->children);
    }
}
