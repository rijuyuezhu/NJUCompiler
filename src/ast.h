#pragma once

#include "general_vec.h"
#include "str.h"
#include "utils.h"

typedef enum AstNodeType {
    ASTNodeInvalid,
    AstNodeBasic,
    AstNodeInt,
    ASTNodeFloat,
    ASTNodeString,
    ASTNodeInner,
} AstNodeType;

typedef struct AstNode {
    AstNodeType type;
    int line_no;
    String grammar_name;

    union {
        int int_val;
        float float_val;
        String str_val;
        VecPtr children;
    };
} AstNode;

AstNode *NSMTD(AstNode, creheap_basic, /, int line_no, String grammar_name);
AstNode *NSMTD(AstNode, creheap_int, /, int line_no, String grammar_name,
               int val);
AstNode *NSMTD(AstNode, creheap_float, /, int line_no, String grammar_name,
               float val);
AstNode *NSMTD(AstNode, creheap_string, /, int line_no, String grammar_name,
               String val);
AstNode *NSMTD(AstNode, creheap_inner, /, int line_no, String grammar_name);

void MTD(AstNode, add_child, /, AstNode *child);
void MTD(AstNode, print_subtree, /, usize depth);
void MTD(AstNode, drop, /);
