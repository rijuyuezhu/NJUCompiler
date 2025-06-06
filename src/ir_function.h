#pragma once

#include "ir_basic_block.h"

struct TaskEngine;
struct Renamer;

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
    ListBoxBB basic_blocks;
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

// call establish/reestablish after this function
void NSMTD(IRFunction, try_strip_gotos, /, IRBasicBlock *bb, usize next_label);
void MTD(IRFunction, add_label, /, usize label);
void MTD(IRFunction, establish, /, struct TaskEngine *engine);
void MTD(IRFunction, reestablish, /);
IRBasicBlock *MTD(IRFunction, label_to_bb, /, usize label);
ListPtr *MTD(IRFunction, get_pred, /, IRBasicBlock *bb);
ListPtr *MTD(IRFunction, get_succ, /, IRBasicBlock *bb);
void MTD(IRFunction, build_str, /, String *builder);

// return: whether or not insert after it
typedef bool (*FunIterStmtCallback)(IRFunction *self, IRBasicBlock *bb,
                                    ListDynIRStmtNode *stmt_it,
                                    void *extra_args);

void MTD(IRFunction, iter_stmt, /, FunIterStmtCallback callback,
         void *extra_args);

typedef bool (*FunIterBBCallback)(IRFunction *self, ListBoxBBNode *bb_it,
                                  void *extra_args);

void MTD(IRFunction, iter_bb, /, FunIterBBCallback callback, void *extra_args);

// call reestablish after this function
bool MTD(IRFunction, remove_dead_bb, /);
// call reestablish after this function, if change any of the goto/if stmts
bool MTD(IRFunction, remove_dead_stmt, /);

void MTD(IRFunction, rename, /, struct Renamer *var_renamer,
         struct Renamer *label_renamer);

DELETED_CLONER(IRFunction, FUNC_STATIC);
DECLARE_CLASS_VEC(VecIRFunction, IRFunction, FUNC_EXTERN);
