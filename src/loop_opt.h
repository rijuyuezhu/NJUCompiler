#pragma once

#include "da_dominator.h"
#include "da_reachdef.h"
#include "ir_stmt.h"

struct IRFunction;

typedef struct LoopInfo {
    struct IRFunction *func;
    struct IRBasicBlock *header;
    struct IRBasicBlock *preheader;
    VecPtr backedge_starts;
    SetPtr nodes;
    VecPtr exits;
    void *aid_engine;
} LoopInfo;

void MTD(LoopInfo, init, /, struct IRFunction *func,
         struct IRBasicBlock *header);
void MTD(LoopInfo, drop, /);
void MTD(LoopInfo, ensure_preheader, /);

DELETED_CLONER(LoopInfo, FUNC_STATIC);

DECLARE_MAPPING(MapHeaderToLoopInfo, struct IRBasicBlock *, LoopInfo,
                FUNC_EXTERN, GENERATOR_PLAIN_KEY, GENERATOR_CLASS_VALUE,
                GENERATOR_PLAIN_COMPARATOR);

typedef struct LoopOpt {
    struct IRFunction *func;

    // some data structures
    usize bb_info_acc;
    MapStmtToBBInfo stmt_to_bb_info;
    MapUSizeToDynIRStmt param_to_stmt;

    // analysis
    DominatorDA dom_da;
    ReachDefDA reach_def_da;

    // loop infos
    MapHeaderToLoopInfo loop_infos;
    VecPtr loop_infos_ordered; // Ordered by increasing nodes size
} LoopOpt;

void MTD(LoopOpt, init, /, struct IRFunction *func);
void MTD(LoopOpt, drop, /);

// utils
bool MTD(LoopOpt, is_dom_bb, /, struct IRBasicBlock *a, struct IRBasicBlock *b);
bool MTD(LoopOpt, is_dom_stmt, /, IRStmtBase *a, IRStmtBase *b);
struct IRBasicBlock *MTD(LoopOpt, get_bb, /, IRStmtBase *stmt);

// prepare
void MTD(LoopOpt, prepare, /);

// optimize; must call fix_preheader after these optimizations;
bool MTD(LoopOpt, invariant_compute_motion, /);
bool MTD(LoopOpt, induction_var_optimize, /);

void MTD(LoopOpt, fix_preheader, /);
