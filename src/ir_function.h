#pragma once
#include "general_container.h"
#include "ir_basic_block.h"
#include "str.h"

struct TaskEngine;

typedef struct DecInfo {
    usize addr;
    int size;
} DecInfo;

DECLARE_MAPPING(MapVarToDecInfo, usize, DecInfo, FUNC_EXTERN,
                GENERATOR_PLAIN_KEY, GENERATOR_PLAIN_VALUE,
                GENERATOR_PLAIN_COMPARATOR);

// IRFunction
typedef struct IRFunction {
    String func_name;
    VecUSize params;
    MapVarToDecInfo var_to_dec_info;
    ListBasicBlock basic_blocks;
    IRBasicBlock *entry;
    IRBasicBlock *exit;
    MapLabelBB label_to_block;
    MapBBToListBB block_pred;
    MapBBToListBB block_succ;
    struct IRProgram *program;
} IRFunction;

void MTD(IRFunction, init, /, String func_name, struct IRProgram *program);
void MTD(IRFunction, drop, /);
void MTD(IRFunction, add_stmt, /, IRStmtBase *stmt);
void MTD(IRFunction, add_label, /, usize label);
void MTD(IRFunction, establish, /, struct TaskEngine *engine);
IRBasicBlock *MTD(IRFunction, label_to_bb, /, usize label);
ListPtr *MTD(IRFunction, get_pred, /, IRBasicBlock *bb);
ListPtr *MTD(IRFunction, get_succ, /, IRBasicBlock *bb);
void MTD(IRFunction, build_str, /, String *builder);

// return: whether or not insert after it
typedef bool (*IterStmtCallback)(IRFunction *self, IRBasicBlock *bb,
                                 ListDynIRStmtNode *stmt_it, void *extra_args);

void MTD(IRFunction, iter_stmt, /, IterStmtCallback callback, void *extra_args);

typedef bool (*IterBBCallback)(IRFunction *self, ListBasicBlockNode *bb_it,
                               void *extra_args);

void MTD(IRFunction, iter_bb, /, IterBBCallback callback, void *extra_args);

bool MTD(IRFunction, remove_dead_stmt, /);

DELETED_CLONER(IRFunction, FUNC_STATIC);
DECLARE_CLASS_VEC(VecIRFunction, IRFunction, FUNC_EXTERN);
