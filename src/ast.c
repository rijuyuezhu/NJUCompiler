#include "ast.h"
#include "debug.h"
#include "str.h"
#include "utils.h"

static AstNode *common_init(int line_no, String grammar_name) {
    AstNode *node = CREOBJRAWHEAP(AstNode);
    node->line_no = line_no;
    node->grammar_name = grammar_name;
    return node;
}
AstNode *NSMTD(AstNode, creheap_basic, /, int line_no, String grammar_name) {
    AstNode *node = common_init(line_no, grammar_name);
    node->type = AstNodeBasic;
    return node;
}

AstNode *NSMTD(AstNode, creheap_int, /, int line_no, String grammar_name,
               int val) {
    AstNode *node = common_init(line_no, grammar_name);
    node->type = AstNodeInt;
    node->int_val = val;
    return node;
}

AstNode *NSMTD(AstNode, creheap_float, /, int line_no, String grammar_name,
               float val) {
    AstNode *node = common_init(line_no, grammar_name);
    node->type = ASTNodeFloat;
    node->float_val = val;
    return node;
}

AstNode *NSMTD(AstNode, creheap_string, /, int line_no, String grammar_name,
               String val) {
    AstNode *node = common_init(line_no, grammar_name);
    node->type = ASTNodeString;
    node->str_val = val;
    return node;
}

AstNode *NSMTD(AstNode, creheap_inner, /, int line_no, String grammar_name) {
    AstNode *node = common_init(line_no, grammar_name);
    node->type = ASTNodeInner;
    node->children = CREOBJ(VecPtr, /);
    return node;
}

void MTD(AstNode, add_child, /, AstNode *child) {
    if (child == NULL) {
        return;
    }
    ASSERT(self->type == ASTNodeInner, "Cannot add child to non-inner node");
    CALL(VecPtr, self->children, push_back, /, child);
}

void MTD(AstNode, print_subtree, /, usize depth) {
    for (usize i = 0; i < depth; i++) {
        printf("  ");
    }
    switch (self->type) {
    case AstNodeBasic:
        printf("%s\n", STRING_C_STR(self->grammar_name));
        break;
    case AstNodeInt:
        printf("%s: %d\n", STRING_C_STR(self->grammar_name), self->int_val);
        break;
    case ASTNodeFloat:
        printf("%s: %f\n", STRING_C_STR(self->grammar_name), self->float_val);
        break;
    case ASTNodeString:
        printf("%s: %s\n", STRING_C_STR(self->grammar_name),
               STRING_C_STR(self->str_val));
        break;
    case ASTNodeInner:
        printf("%s (%d)\n", STRING_C_STR(self->grammar_name), self->line_no);
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
    DROPOBJ(String, self->grammar_name);
    if (self->type == ASTNodeString) {
        DROPOBJ(String, self->str_val);
    } else if (self->type == ASTNodeInner) {
        for (usize i = 0; i < self->children.size; i++) {
            DROPOBJHEAP(AstNode, self->children.data[i]);
        }
        DROPOBJ(VecPtr, self->children);
    }
    self->type = ASTNodeInvalid;
}
