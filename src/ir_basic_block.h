#pragma once
#include "general_container.h"
#include "ir_stmt.h"

// IRBasicBlock
typedef struct IRBasicBlock {
    ListDynIRStmt stmts;
    usize label;
    bool is_dead;
} IRBasicBlock;

void MTD(IRBasicBlock, init, /, usize label);
void MTD(IRBasicBlock, add_stmt, /, IRStmtBase *stmt);
void MTD(IRBasicBlock, drop, /);
void MTD(IRBasicBlock, build_str, /, String *builder);

DELETED_CLONER(IRBasicBlock, FUNC_STATIC);

DECLARE_LIST(ListBasicBlock, IRBasicBlock, FUNC_EXTERN, GENERATOR_CLASS_VALUE);
DECLARE_MAPPING(MapLabelBB, usize, IRBasicBlock *, FUNC_EXTERN,
                GENERATOR_PLAIN_KEY, GENERATOR_PLAIN_VALUE,
                GENERATOR_PLAIN_COMPARATOR);
DECLARE_MAPPING(MapBBToListBB, IRBasicBlock *, ListPtr, FUNC_EXTERN,
                GENERATOR_PLAIN_KEY, GENERATOR_CLASS_VALUE,
                GENERATOR_PLAIN_COMPARATOR);
