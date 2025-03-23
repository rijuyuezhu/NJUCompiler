#include "ast.h"
#include "debug.h"
#include "str.h"
#include "utils.h"

static AstNode *common_init(int line_no, GrammarSymbol grammar_symbol,
                            const char *grammar_symbol_str) {
    AstNode *node = CREOBJRAWHEAP(AstNode);
    node->grammar_symbol = grammar_symbol;
    node->line_no = line_no;
    node->grammar_symbol_str = grammar_symbol_str;
    return node;
}
AstNode *NSMTD(AstNode, creheap_basic, /, int line_no,
               GrammarSymbol grammar_symbol, const char *grammar_symbol_str) {
    AstNode *node = common_init(line_no, grammar_symbol, grammar_symbol_str);
    node->kind = AstNodeBasic;
    return node;
}

AstNode *NSMTD(AstNode, creheap_int, /, int line_no,
               GrammarSymbol grammar_symbol, const char *grammar_symbol_str,
               int val) {
    AstNode *node = common_init(line_no, grammar_symbol, grammar_symbol_str);
    node->kind = AstNodeInt;
    node->int_val = val;
    return node;
}

AstNode *NSMTD(AstNode, creheap_float, /, int line_no,
               GrammarSymbol grammar_symbol, const char *grammar_symbol_str,
               float val) {
    AstNode *node = common_init(line_no, grammar_symbol, grammar_symbol_str);
    node->kind = ASTNodeFloat;
    node->float_val = val;
    return node;
}

AstNode *NSMTD(AstNode, creheap_string, /, int line_no,
               GrammarSymbol grammar_symbol, const char *grammar_symbol_str,
               String val) {
    AstNode *node = common_init(line_no, grammar_symbol, grammar_symbol_str);
    node->kind = ASTNodeString;
    node->str_val = val;
    return node;
}

AstNode *NSMTD(AstNode, creheap_relop, /, int line_no,
               GrammarSymbol grammar_symbol, const char *grammar_symbol_str,
               RelopKind val) {
    AstNode *node = common_init(line_no, grammar_symbol, grammar_symbol_str);
    node->kind = ASTNodeRelop;
    node->relop_val = val;
    return node;
}

AstNode *NSMTD(AstNode, creheap_inner, /, int line_no,
               GrammarSymbol grammar_symbol, const char *grammar_symbol_str) {
    AstNode *node = common_init(line_no, grammar_symbol, grammar_symbol_str);
    node->kind = ASTNodeInner;
    node->children = CREOBJ(VecPtr, /);
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
        printf("%s\n", self->grammar_symbol_str);
        break;
    case AstNodeInt:
        printf("%s: %d\n", self->grammar_symbol_str, self->int_val);
        break;
    case ASTNodeFloat:
        printf("%s: %f\n", self->grammar_symbol_str, self->float_val);
        break;
    case ASTNodeString:
        printf("%s: %s\n", self->grammar_symbol_str,
               STRING_C_STR(self->str_val));
        break;
    case ASTNodeInner:
        printf("%s (%d)\n", self->grammar_symbol_str, self->line_no);
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
