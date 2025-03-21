#include "ast.h"
#include "debug.h"
#include "str.h"
#include "utils.h"

static AstNode *common_init(int line_no, int grammar_symbol,
                            const char *grammar_name) {
    AstNode *node = CREOBJRAWHEAP(AstNode);
    node->grammar_symbol = grammar_symbol;
    node->line_no = line_no;
    node->grammar_name = grammar_name;
    return node;
}
AstNode *NSMTD(AstNode, creheap_basic, /, int line_no, int grammar_symbol,
               const char *grammar_name) {
    AstNode *node = common_init(line_no, grammar_symbol, grammar_name);
    node->kind = AstNodeBasic;
    return node;
}

AstNode *NSMTD(AstNode, creheap_int, /, int line_no, int grammar_symbol,
               const char *grammar_name, int val) {
    AstNode *node = common_init(line_no, grammar_symbol, grammar_name);
    node->kind = AstNodeInt;
    node->int_val = val;
    return node;
}

AstNode *NSMTD(AstNode, creheap_float, /, int line_no, int grammar_symbol,
               const char *grammar_name, float val) {
    AstNode *node = common_init(line_no, grammar_symbol, grammar_name);
    node->kind = ASTNodeFloat;
    node->float_val = val;
    return node;
}

AstNode *NSMTD(AstNode, creheap_string, /, int line_no, int grammar_symbol,
               const char *grammar_name, String val) {
    AstNode *node = common_init(line_no, grammar_symbol, grammar_name);
    node->kind = ASTNodeString;
    node->str_val = val;
    return node;
}

AstNode *NSMTD(AstNode, creheap_relop, /, int line_no, int grammar_symbol,
               const char *grammar_name, RelopKind val) {
    AstNode *node = common_init(line_no, grammar_symbol, grammar_name);
    node->kind = ASTNodeRelop;
    node->relop_val = val;
    return node;
}

AstNode *NSMTD(AstNode, creheap_inner, /, int line_no, int grammar_symbol,
               const char *grammar_name) {
    AstNode *node = common_init(line_no, grammar_symbol, grammar_name);
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
    for (usize i = 0; i < depth; i++) {
        printf("  ");
    }
    switch (self->kind) {
    case AstNodeBasic:
    case ASTNodeRelop:
        printf("%s\n", self->grammar_name);
        break;
    case AstNodeInt:
        printf("%s: %d\n", self->grammar_name, self->int_val);
        break;
    case ASTNodeFloat:
        printf("%s: %f\n", self->grammar_name, self->float_val);
        break;
    case ASTNodeString:
        printf("%s: %s\n", self->grammar_name, STRING_C_STR(self->str_val));
        break;
    case ASTNodeInner:
        printf("%s (%d)\n", self->grammar_name, self->line_no);
        for (usize i = 0; i < self->children.size; i++) {
            CALL(AstNode, *(AstNode *)self->children.data[i], print_subtree, /,
                 depth + 1);
        }
        break;
    default:
        ASSERT(false, "Invalid AstNodeType");
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
